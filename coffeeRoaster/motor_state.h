#pragma once

#include <mutex>
#include <atomic>

// Shared state between the motor control loop (main thread) and the
// socket command server (listener thread). All access is protected by mtx.
//
// The main thread is the only writer of: commandedRpm, motorConnected, hasAlert.
// The socket thread is the only writer of: targetRpm.
// Both threads read both sets of fields.

struct MotorState {
    std::mutex mtx;

    // Written by socket thread, read by main thread
    double targetRpm = 0.0;
    bool disableRequested = false;
    bool enableRequested = false;

    // Written by main thread, read by socket thread
    double commandedRpm = 0.0;
    bool motorConnected = false;
    bool motorEnabled = false;
    bool hasAlert = false;

    // Shutdown flag - set by signal handler, read by both threads
    std::atomic<bool> running{true};
};
