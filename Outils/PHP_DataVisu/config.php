<?php
/**
 * config.php
 *
 * Configuration parameters for thermostat.php
 */

// Directory where monthly CSV files are stored.
// Must be writable by the web server.
return [
    'data_dir'   => __DIR__ . '/data',

    // Timezone of the incoming request/device.
    'source_tz'  => 'Europe/Paris',

    // Target timezone written in the CSV (GMT/UTC).
    'target_tz'  => 'UTC',

    // Secret API key that must be sent in the X-API-Key header.
    'api_key'    => 'change-me-to-a-strong-secret',

    // Artificial delay in milliseconds applied before verifying the API key
    // to help mitigate timing attacks.
    'key_delay_ms' => 500,
];
