#pragma once

// Motor parameters
constexpr double MAX_RPM = 60.0;                // Max settable velocity (drum ~15 RPM typical)
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
