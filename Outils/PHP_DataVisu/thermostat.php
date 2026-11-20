<?php
/**
 * thermostat.php
 *
 * Accepts a POST request with a 'vector' parameter composed of repeating tuples:
 *   temperature,hour,minute,heating
 *
 * Rules:
 * - The request must provide the secret API key in the X-API-Key header.
 * - The vector is sorted chronologically and the last tuple represents the current time.
 * - Duplicate (timestamp, temperature, heating) rows are removed.
 * - Each reading is converted from France local time to GMT/UTC.
 * - One CSV file is maintained per month in data/thermostat_YYYY-MM.csv
 *   with columns: timestamp (GMT), temperature, heating.
 */

// ---------------------------------------------------------------------------
// 0. Load configuration
// ---------------------------------------------------------------------------
$configPath = __DIR__ . '/config.php';
if (!file_exists($configPath)) {
    http_response_code(500);
    header('Content-Type: application/json');
    echo json_encode(['error' => 'Configuration file missing']);
    exit;
}

$config = require $configPath;

$required = ['data_dir', 'source_tz', 'target_tz', 'api_key', 'key_delay_ms'];
foreach ($required as $key) {
    if (!array_key_exists($key, $config)) {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(['error' => "Missing configuration key: $key"]);
        exit;
    }
}

// Set the source timezone (when the request is received).
date_default_timezone_set($config['source_tz']);

const CSV_HEADER = "timestamp,temperature,heating\n";

/**
 * Return a 401 response and exit.
 */
function unauthorized(string $message = 'Unauthorized'): void {
    http_response_code(401);
    header('Content-Type: application/json');
    echo json_encode(['error' => $message]);
    exit;
}

// ---------------------------------------------------------------------------
// 1. Validate request method and authenticate via X-API-Key header
// ---------------------------------------------------------------------------
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    header('Content-Type: application/json');
    echo json_encode(['error' => 'Only POST requests are allowed']);
    exit;
}

// Small constant-time delay before checking the key to reduce timing side-channels.
usleep((int) $config['key_delay_ms'] * 1000);

$providedKey = $_SERVER['HTTP_X_API_KEY'] ?? '';
$expectedKey = (string) $config['api_key'];

if ($expectedKey === '') {
    unauthorized('API key not configured');
}

// Constant-time comparison to avoid timing attacks.
if (!hash_equals($expectedKey, $providedKey)) {
    unauthorized('Invalid API key');
}

$vector = $_POST['hist'] ?? '';
if ($vector === '') {
    http_response_code(400);
    header('Content-Type: application/json');
    echo json_encode(['error' => 'Missing "hist" parameter']);
    exit;
}

// ---------------------------------------------------------------------------
// 2. Parse vector into tuples (temperature, hour, minute, heating)
// ---------------------------------------------------------------------------
$parts = array_map('trim', explode(',', $vector));
$count = count($parts);

if ($count === 0 || $count % 4 !== 0) {
    http_response_code(400);
    header('Content-Type: application/json');
    echo json_encode(['error' => 'Vector length must be a multiple of 4']);
    exit;
}

$tuples = [];
$parisTz  = new DateTimeZone($config['source_tz']);

for ($i = 0; $i < $count; $i += 4) {
    $temp    = floatval($parts[$i]);
    $hour    = intval($parts[$i + 1]);
    $minute  = intval($parts[$i + 2]);
    $heating = intval($parts[$i + 3]);

    // Basic validation
    if ($hour < 0 || $hour > 23 || $minute < 0 || $minute > 59) {
        continue;
    }
    if ($heating !== 0 && $heating !== 1) {
        continue;
    }

    $tuples[] = [
        'temp'    => $temp,
        'hour'    => $hour,
        'minute'  => $minute,
        'heating' => $heating,
    ];
}

if (empty($tuples)) {
    http_response_code(400);
    header('Content-Type: application/json');
    echo json_encode(['error' => 'No valid tuples found in vector']);
    exit;
}

// ---------------------------------------------------------------------------
// 3. Assign real dates in source timezone
//    The last tuple is the current time. Going backwards, if a tuple's clock
//    time is >= the next tuple's clock time, it belongs to the previous day.
//    Raw tuples must not be de-duplicated: the same clock time and temperature
//    can legitimately occur again on another day.
// ---------------------------------------------------------------------------
$now = new DateTime('now', $parisTz);

$lastIndex = count($tuples) - 1;
$lastDateTime = clone $now;
$lastDateTime->setTime($tuples[$lastIndex]['hour'], $tuples[$lastIndex]['minute'], 0);

// Just after midnight, the latest record can still belong to the previous day
// (for example 23:59 when the request is received at 00:01). If the last tuple
// is clearly ahead of the current time (more than 5 minutes, to tolerate small
// clock skew), it is anchored on the previous day instead.
$lastTs = $lastDateTime->getTimestamp();
$nowTs  = $now->getTimestamp();
if ($lastTs > $nowTs + 300) {
    $lastDateTime->modify('-1 day');
}
$tuples[$lastIndex]['datetime_france'] = $lastDateTime;

for ($i = $lastIndex - 1; $i >= 0; $i--) {
    $next = $tuples[$i + 1];
    $dt   = clone $next['datetime_france'];
    $dt->setTime($tuples[$i]['hour'], $tuples[$i]['minute'], 0);

    // Crossed midnight (or spans several days)
    while ($dt >= $next['datetime_france']) {
        $dt->modify('-1 day');
    }

    $tuples[$i]['datetime_france'] = $dt;
}

// ---------------------------------------------------------------------------
// 4. Convert to target timezone (GMT/UTC) and build unique rows
// ---------------------------------------------------------------------------
$utcTz    = new DateTimeZone($config['target_tz']);
$unique   = [];

foreach ($tuples as $t) {
    $dt = clone $t['datetime_france'];
    $dt->setTimezone($utcTz);

    $ts      = $dt->format('Y-m-d\TH:i:sP'); // ISO 8601 with +00:00 offset
    $temp    = number_format($t['temp'], 2, '.', '');
    $heating = $t['heating'];
    $month   = $dt->format('Y-m');

    $key = $ts . '|' . $temp . '|' . $heating;

    $unique[$key] = [
        'ts'      => $ts,
        'temp'    => $temp,
        'heating' => $heating,
        'month'   => $month,
    ];
}

$entries = array_values($unique);

// ---------------------------------------------------------------------------
// 5. Group entries by month and merge into existing CSV files
// ---------------------------------------------------------------------------
$dataDir = $config['data_dir'];
if (!is_dir($dataDir)) {
    mkdir($dataDir, 0755, true);
}

$byMonth = [];
foreach ($entries as $e) {
    $byMonth[$e['month']][] = $e;
}

foreach ($byMonth as $month => $monthEntries) {
    $file = $dataDir . '/thermostat_' . $month . '.csv';
    $existing = [];

    // Read existing rows to avoid duplicates
    if (file_exists($file)) {
        $lines = file($file, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
        $isHeader = true;
        foreach ($lines as $line) {
            if ($isHeader) {
                $isHeader = false;
                continue;
            }
            $cols = str_getcsv($line);
            if (count($cols) >= 3) {
                // Also remove future samples produced by the old midnight bug.
                $timestamp = strtotime($cols[0]);
                if ($timestamp !== false && $timestamp > $now->getTimestamp()) {
                    continue;
                }
                $key = $cols[0] . '|' . $cols[1] . '|' . $cols[2];
                $existing[$key] = [$cols[0], $cols[1], $cols[2]];
            }
        }
    }

    // Merge new entries
    foreach ($monthEntries as $e) {
        $key = $e['ts'] . '|' . $e['temp'] . '|' . $e['heating'];
        $existing[$key] = [$e['ts'], $e['temp'], $e['heating']];
    }

    // Sort by timestamp
    ksort($existing);

    // Write atomically with a file lock
    $fp = fopen($file, 'c');
    if (!$fp) {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(['error' => 'Unable to open file: ' . $file]);
        exit;
    }

    if (flock($fp, LOCK_EX)) {
        ftruncate($fp, 0);
        fwrite($fp, CSV_HEADER);
        foreach ($existing as $row) {
            fputcsv($fp, $row);
        }
        fflush($fp);
        flock($fp, LOCK_UN);
    } else {
        http_response_code(500);
        header('Content-Type: application/json');
        echo json_encode(['error' => 'Unable to lock file: ' . $file]);
        exit;
    }

    fclose($fp);
}

// ---------------------------------------------------------------------------
// 6. Response
// ---------------------------------------------------------------------------
header('Content-Type: application/json');
echo json_encode([
    'success' => true,
    'inserted' => count($entries),
]);
