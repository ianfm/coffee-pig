#include "config.h"
#include "motor_state.h"
#include "motor_controller.h"
#include "socket_server.h"

#include <cstdio>
#include <cmath>
#include <csignal>
#include <thread>
#include <unistd.h>

static MotorState g_state;

static void signalHandler(int) {
    g_state.running.store(false);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // Start the Unix socket command server on a background thread
    SocketServer server(g_state);
    if (!server.start(SOCKET_PATH)) {
        fprintf(stderr, "Failed to start socket server on %s\n", SOCKET_PATH);
        return 1;
    }
    std::thread serverThread([&server]() { server.run(); });

    // Initialize motor
    MotorController motor;
    if (!motor.init()) {
        fprintf(stderr, "Failed to initialize motor controller.\n");
        // Keep running so the web UI can report the error and we can retry
    }

    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        g_state.motorConnected = motor.isConnected();
    }

    double lastCommandedRpm = 0.0;
    int loopCount = 0;

    printf("Motor daemon running (control rate %d Hz, max %.0f RPM)\n",
           (int)(1e6 / LOOP_PERIOD_US), MAX_RPM);

    while (g_state.running.load()) {
        double target;
        {
            std::lock_guard<std::mutex> lock(g_state.mtx);
            target = g_state.targetRpm;
        }

        // Send velocity command if target changed
        // setVelocity(0) disables, setVelocity(>0) auto-enables
        if (motor.isConnected()) {
            if (fabs(target - lastCommandedRpm) > RPM_CHANGE_THRESHOLD) {
                if (motor.setVelocity(target)) {
                    lastCommandedRpm = target;
                    std::lock_guard<std::mutex> lock(g_state.mtx);
                    g_state.commandedRpm = target;
                }
            }
        }

        // Periodic alert check
        if (++loopCount >= ALERT_CHECK_INTERVAL) {
            loopCount = 0;
            if (motor.isConnected()) {
                bool ok = motor.checkAlerts();
                std::lock_guard<std::mutex> lock(g_state.mtx);
                g_state.hasAlert = !ok;
            }
        }

        // Update connection/enable state
        {
            std::lock_guard<std::mutex> lock(g_state.mtx);
            g_state.motorConnected = motor.isConnected();
            g_state.motorEnabled = motor.isEnabled();
        }

        // Reconnect if disconnected
        if (!motor.isConnected()) {
            fprintf(stderr, "Motor disconnected, retrying in %ds...\n",
                    RECONNECT_DELAY_S);
            sleep(RECONNECT_DELAY_S);
            if (motor.reconnect()) {
                lastCommandedRpm = 0.0;
                std::lock_guard<std::mutex> lock(g_state.mtx);
                g_state.motorConnected = true;
                g_state.commandedRpm = 0.0;
                printf("Reconnected.\n");
            }
            continue; // skip the usleep since we already slept
        }

        usleep(LOOP_PERIOD_US);
    }

    printf("\nShutdown requested...\n");
    motor.shutdown();
    server.stop();
    serverThread.join();
    printf("Done.\n");
    return 0;
}
