#pragma once

// Motor parameters
constexpr double MAX_RPM = 300.0;               // Motor shaft max (e.g. 30 RPM output at 10:1)
constexpr double ACC_LIM_RPM_PER_SEC = 100.0;   // Gentle acceleration for a loaded drum
constexpr double RPM_CHANGE_THRESHOLD = 0.1;    // Min change to issue new command

// Control loop
constexpr int LOOP_PERIOD_US = 50000;   // 50ms = 20 Hz motor control rate

// Motor enable timeout
constexpr double ENABLE_TIMEOUT_MS = 10000.0;

// Error recovery
constexpr int RECONNECT_DELAY_S = 5;
constexpr int ALERT_CHECK_INTERVAL = 20; // Check alerts every N loops (~1s at 20Hz)

// IPC socket
constexpr const char *SOCKET_PATH = "/run/coffee-roaster/motor.sock";
