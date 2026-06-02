#pragma once

#include "pubSysCls.h"

class MotorController {
public:
    MotorController();

    bool init();
    bool setVelocity(double rpm);
    bool checkAlerts();
    void shutdown();
    bool reconnect();

    bool isConnected() const { return m_connected; }

private:
    sFnd::SysManager *m_mgr;
    sFnd::INode *m_node;
    bool m_connected;
};
