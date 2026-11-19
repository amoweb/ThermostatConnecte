<?php
/**
 * data.php
 *
 * Read-only JSON API used by the visualization page (index.html).
 *
 * The samples are stored in monthly CSV files (see thermostat.php):
 *   data/thermostat_YYYY-MM.csv with columns: timestamp (GMT), temperature, heating
 *
 * Endpoints (GET only, JSON responses):
 *
 * 1) data.php
 *    Overview of the available data:
 *    { "months": ["2026-10", "2026-11"],
 *      "min": 1763328000000,   // epoch ms of the oldest sample
 *      "max": 1763352000000 }  // epoch ms of the newest sample
 *    "min"/"max" are null when no sample exists yet.
 *
 * 2) data.php?from=<epoch_ms>&to=<epoch_ms>
 *    Samples within [from, to] (both included):
 *    { "from": ..., "to": ..., "count": n, "points": [[epoch_ms, temp, heating], ...] }
 *    - The span is clamped server-side to 92 days (about 3 months), so at
 *      most three monthly files are ever read or transferred.
 *    - Points are sorted by timestamp and de-duplicated (last one wins).
 */

declare(strict_types=1);

const MAX_SPAN_MS = 92 * 86400000; // hard limit: about 3 months

/**
 * Send a JSON error and exit.
 */
function fail(int $code, string $message): void
{
    http_response_code($code);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode(['error' => $message]);
    exit;
}

$configPath = __DIR__ . '/config.php';
if (!is_file($configPath)) {
    fail(500, 'Configuration file missing');
}
$config = require $configPath;
if (empty($config['data_dir'])) {
    fail(500, "Missing configuration key: data_dir");
}
$dataDir = (string) $config['data_dir'];

header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store');

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
    fail(405, 'Only GET requests are allowed');
}

/**
 * Parse a CSV timestamp like "2026-11-18T01:06:00+00:00" into epoch ms.
 */
function parseTimestampMs(string $ts): ?int
{
    $epoch = strtotime($ts);
    return $epoch === false ? null : $epoch * 1000;
}

/**
 * Read the samples of one CSV file as [epoch_ms, temperature, heating] rows.
 * A shared lock is taken: files are rewritten by thermostat.php under LOCK_EX.
 * When $from/$to are given, only rows within [from, to] are returned.
 */
function readSamples(string $file, ?int $from = null, ?int $to = null): array
{
    $samples = [];
    $fp = @fopen($file, 'r');
    if ($fp === false) {
        return $samples;
    }
    @flock($fp, LOCK_SH);
    try {
        $isHeader = true;
        while (($cols = fgetcsv($fp)) !== false) {
            if ($isHeader) {
                $isHeader = false;
                continue;
            }
            if (count($cols) < 3 || !is_numeric($cols[1])) {
                continue;
            }
            $tsMs = parseTimestampMs(trim($cols[0]));
            if ($tsMs === null) {
                continue;
            }
            if ($from !== null && ($tsMs < $from || $tsMs > $to)) {
                continue;
            }
            $samples[] = [$tsMs, (float) $cols[1], ((int) $cols[2] === 1) ? 1 : 0];
        }
    } finally {
        @flock($fp, LOCK_UN);
        fclose($fp);
    }
    return $samples;
}

// ---------------------------------------------------------------------------
// List the available month files
// ---------------------------------------------------------------------------
$months = [];
if (is_dir($dataDir)) {
    foreach (scandir($dataDir) as $name) {
        if (is_file($dataDir . '/' . $name)
            && preg_match('/^thermostat_(\d{4}-\d{2})\.csv$/', $name, $m)) {
            $months[] = $m[1];
        }
    }
    sort($months);
}

// ---------------------------------------------------------------------------
// 1) Overview: months + extent of the data
// ---------------------------------------------------------------------------
$hasFrom = array_key_exists('from', $_GET);
$hasTo   = array_key_exists('to', $_GET);

if (!$hasFrom && !$hasTo) {
    $min = null;
    $max = null;
    // Oldest sample: first non-empty file.
    foreach ($months as $month) {
        $rows = readSamples($dataDir . '/thermostat_' . $month . '.csv');
        if ($rows !== []) {
            $min = $rows[0][0];
            break;
        }
    }
    // Newest sample: last non-empty file.
    foreach (array_reverse($months) as $month) {
        $rows = readSamples($dataDir . '/thermostat_' . $month . '.csv');
        if ($rows !== []) {
            $max = $rows[count($rows) - 1][0];
            break;
        }
    }
    echo json_encode(['months' => $months, 'min' => $min, 'max' => $max]);
    exit;
}

// ---------------------------------------------------------------------------
// 2) Samples of a time window (clamped to the 3-month limit)
// ---------------------------------------------------------------------------
if (!$hasFrom || !$hasTo) {
    fail(400, 'Both "from" and "to" (epoch ms) are required');
}

$from = filter_input(INPUT_GET, 'from', FILTER_VALIDATE_INT);
$to   = filter_input(INPUT_GET, 'to', FILTER_VALIDATE_INT);
if ($from === false || $from === null || $to === false || $to === null) {
    fail(400, '"from" and "to" must be integers (epoch ms)');
}
$from = (int) $from;
$to   = (int) $to;
if ($from > $to) {
    fail(400, '"from" must be <= "to"');
}

// Never trust the client: clamp the span to the 3-month limit.
if ($to - $from > MAX_SPAN_MS) {
    $to = $from + MAX_SPAN_MS;
}

// Read every monthly file overlapping the window (file names are UTC months).
$utc        = new DateTimeZone('UTC');
$firstMonth = new DateTimeImmutable(
    (new DateTimeImmutable('@' . intdiv($from, 1000)))->setTimezone($utc)->format('Y-m') . '-01',
    $utc
);
$lastMonth = new DateTimeImmutable(
    (new DateTimeImmutable('@' . intdiv($to, 1000)))->setTimezone($utc)->format('Y-m') . '-01',
    $utc
);

$points = [];
for ($m = $firstMonth; $m <= $lastMonth; $m = $m->modify('first day of next month')) {
    $file = $dataDir . '/thermostat_' . $m->format('Y-m') . '.csv';
    if (!is_file($file)) {
        continue;
    }
    foreach (readSamples($file, $from, $to) as $sample) {
        // De-duplicate by timestamp (keep the last occurrence).
        $points[$sample[0]] = $sample;
    }
}
ksort($points);

echo json_encode([
    'from'   => $from,
    'to'     => $to,
    'count'  => count($points),
    'points' => array_values($points),
]);
