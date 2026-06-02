#pragma once

#include "motor_state.h"
#include <string>

// Listens on a Unix domain socket and accepts simple text commands
// from the web server (or any client, e.g. socat for debugging).
//
// Protocol (newline-terminated, one command per connection):
//   SET <rpm>\n   → OK <target> <commanded> <connected> <alert> <enabled>\n
//   GET\n         → OK <target> <commanded> <connected> <alert> <enabled>\n
//   STOP\n        → OK 0.0 <commanded> <connected> <alert> <enabled>\n
//   DISABLE\n     → OK 0.0 <commanded> <connected> <alert> <enabled>\n
//   ENABLE\n      → OK <target> <commanded> <connected> <alert> <enabled>\n
//   ERR <msg>\n   (on parse failure)

class SocketServer {
public:
    explicit SocketServer(MotorState &state);
    ~SocketServer();

    bool start(const char *path);
    void run();   // Blocking — call from a dedicated thread
    void stop();

private:
    void handleClient(int clientFd);
    std::string formatStatus();

    int m_listenFd;
    const char *m_path;
    MotorState &m_state;
};
