#include "socket_server.h"
#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <poll.h>

SocketServer::SocketServer(MotorState &state)
    : m_listenFd(-1), m_path(nullptr), m_state(state) {}

SocketServer::~SocketServer() {
    stop();
}

bool SocketServer::start(const char *path) {
    m_path = path;

    // Remove stale socket from a previous crash
    unlink(path);

    m_listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_listenFd < 0) {
        perror("socket");
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(m_listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(m_listenFd);
        m_listenFd = -1;
        return false;
    }

    // Allow the web server (running as same user) to connect
    chmod(path, 0660);

    if (listen(m_listenFd, 4) < 0) {
        perror("listen");
        close(m_listenFd);
        m_listenFd = -1;
        return false;
    }

    printf("Socket server listening on %s\n", path);
    return true;
}

void SocketServer::run() {
    while (m_state.running.load()) {
        // Poll with a timeout so we can check the shutdown flag
        struct pollfd pfd;
        pfd.fd = m_listenFd;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, 500); // 500ms timeout
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        if (ret == 0) continue; // timeout, check running flag

        int clientFd = accept(m_listenFd, nullptr, nullptr);
        if (clientFd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        handleClient(clientFd);
        close(clientFd);
    }
}

void SocketServer::stop() {
    if (m_listenFd >= 0) {
        close(m_listenFd);
        m_listenFd = -1;
    }
    if (m_path)
        unlink(m_path);
}

void SocketServer::handleClient(int clientFd) {
    // Read one line (command + newline), max 256 bytes
    char buf[256];
    ssize_t n = read(clientFd, buf, sizeof(buf) - 1);
    if (n <= 0) return;
    buf[n] = '\0';

    // Strip trailing newline/whitespace
    while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r' || buf[n-1] == ' '))
        buf[--n] = '\0';

    std::string response;

    if (strncmp(buf, "SET ", 4) == 0) {
        char *endp;
        double rpm = strtod(buf + 4, &endp);
        if (endp == buf + 4) {
            response = "ERR bad number\n";
        } else {
            if (rpm > MAX_RPM) rpm = MAX_RPM;
            if (rpm < -MAX_RPM) rpm = -MAX_RPM;
            {
                std::lock_guard<std::mutex> lock(m_state.mtx);
                m_state.targetRpm = rpm;
            }
            response = "OK " + formatStatus() + "\n";
        }
    } else if (strncmp(buf, "GET", 3) == 0) {
        response = "OK " + formatStatus() + "\n";
    } else if (strncmp(buf, "STOP", 4) == 0) {
        {
            std::lock_guard<std::mutex> lock(m_state.mtx);
            m_state.targetRpm = 0.0;
        }
        response = "OK " + formatStatus() + "\n";
    } else if (strncmp(buf, "RECONNECT", 9) == 0) {
        {
            std::lock_guard<std::mutex> lock(m_state.mtx);
            m_state.reconnectRequested = true;
        }
        response = "OK " + formatStatus() + "\n";
    } else {
        response = "ERR unknown command\n";
    }

    if (write(clientFd, response.c_str(), response.size()) < 0)
        perror("SocketServer: write to client failed");
}

std::string SocketServer::formatStatus() {
    std::lock_guard<std::mutex> lock(m_state.mtx);
    char buf[128];
    snprintf(buf, sizeof(buf), "%.1f %.1f %d %d %d",
             m_state.targetRpm,
             m_state.commandedRpm,
             m_state.motorConnected ? 1 : 0,
             m_state.hasAlert ? 1 : 0,
             m_state.motorEnabled ? 1 : 0);
    return std::string(buf);
}
