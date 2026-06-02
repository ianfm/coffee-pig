#pragma once

#include "pubSysCls.h"

class MotorController {
public:
    MotorController();

    bool init();
    bool setVelocity(double rpm);
    bool disable();
    bool enable();
    bool checkAlerts();
    void shutdown();
    bool reconnect();

    bool isConnected() const { return m_connected; }
    bool isEnabled() const { return m_enabled; }

private:
    sFnd::SysManager *m_mgr;
    sFnd::INode *m_node;
    bool m_connected;
    bool m_enabled;
};
