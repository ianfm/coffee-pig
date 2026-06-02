#include "motor_controller.h"
#include "config.h"

#include <cstdio>
#include <vector>
#include <string>

using namespace sFnd;

MotorController::MotorController()
    : m_mgr(nullptr), m_node(nullptr), m_connected(false), m_enabled(false) {}

bool MotorController::init() {
    m_mgr = SysManager::Instance();

    std::vector<std::string> comHubPorts;
    SysManager::FindComHubPorts(comHubPorts);
    if (comHubPorts.empty()) {
        fprintf(stderr, "No SC4-HUB found. Check USB connection and 24V power.\n");
        return false;
    }

    printf("Found SC4-HUB on %s\n", comHubPorts[0].c_str());
    m_mgr->ComHubPort(0, comHubPorts[0].c_str());

    try {
        m_mgr->PortsOpen(1);
    } catch (mnErr &e) {
        fprintf(stderr, "Failed to open port: err=0x%08x msg=%s\n",
                e.ErrorCode, e.ErrorMsg);
        return false;
    }

    IPort &port = m_mgr->Ports(0);
    if (port.NodeCount() < 1) {
        fprintf(stderr, "No nodes found on port 0.\n");
        m_mgr->PortsClose();
        return false;
    }

    m_node = &port.Nodes(0);
    printf("Node[0]: model=%s serial=%d fw=%s\n",
           m_node->Info.Model.Value(),
           m_node->Info.SerialNumber.Value(),
           m_node->Info.FirmwareVersion.Value());

    // Configure units
    m_node->AccUnit(INode::RPM_PER_SEC);
    m_node->VelUnit(INode::RPM);
    m_node->Motion.AccLimit = ACC_LIM_RPM_PER_SEC;

    // Clear alerts and enable
    m_node->Motion.NodeStopClear();
    m_node->Status.AlertsClear();
    m_node->EnableReq(true);

    // Wait for node ready
    double timeout = m_mgr->TimeStampMsec() + ENABLE_TIMEOUT_MS;
    while (!m_node->Motion.IsReady()) {
        if (m_mgr->TimeStampMsec() > timeout) {
            fprintf(stderr, "Timed out waiting for node to enable.\n");
            m_mgr->PortsClose();
            return false;
        }
    }

    printf("Node enabled and ready.\n");
    m_connected = true;
    m_enabled = true;
    return true;
}

bool MotorController::setVelocity(double rpm) {
    if (!m_connected || !m_node)
        return false;

    try {
        if (rpm <= 0.0) {
            // Disable: stop and release the motor
            if (m_enabled) {
                m_node->Motion.MoveVelStart(0);
                m_node->EnableReq(false);
                m_enabled = false;
                printf("Motor disabled.\n");
            }
        } else {
            // Enable if needed, then spin
            if (!m_enabled) {
                m_node->Motion.NodeStopClear();
                m_node->Status.AlertsClear();
                m_node->EnableReq(true);

                double timeout = m_mgr->TimeStampMsec() + ENABLE_TIMEOUT_MS;
                while (!m_node->Motion.IsReady()) {
                    if (m_mgr->TimeStampMsec() > timeout) {
                        fprintf(stderr, "Timed out waiting for enable.\n");
                        return false;
                    }
                }
                m_enabled = true;
                printf("Motor enabled.\n");
            }
            m_node->Motion.MoveVelStart(rpm);
        }
    } catch (mnErr &e) {
        fprintf(stderr, "setVelocity failed: err=0x%08x msg=%s\n",
                e.ErrorCode, e.ErrorMsg);
        m_connected = false;
        return false;
    }

    return true;
}

bool MotorController::checkAlerts() {
    if (!m_connected || !m_node)
        return false;

    try {
        m_node->Status.Alerts.Refresh();
        if (!m_node->Status.Alerts.Value().isInAlert())
            return true;

        char alertList[256];
        m_node->Status.Alerts.Value().StateStr(alertList, sizeof(alertList));
        fprintf(stderr, "Alert detected: %s\n", alertList);

        // Clear e-stop if present
        if (m_node->Status.Alerts.Value().cpm.Common.EStopped) {
            fprintf(stderr, "Clearing E-Stop...\n");
            m_node->Motion.NodeStopClear();
        }

        // Attempt to clear remaining alerts
        m_node->Status.AlertsClear();
        m_node->Status.Alerts.Refresh();

        if (m_node->Status.Alerts.Value().isInAlert()) {
            m_node->Status.Alerts.Value().StateStr(alertList, sizeof(alertList));
            fprintf(stderr, "Serious alerts remain: %s\n", alertList);
            return false;
        }

        printf("All alerts cleared.\n");
        return true;
    } catch (mnErr &e) {
        fprintf(stderr, "Alert check failed: err=0x%08x msg=%s\n",
                e.ErrorCode, e.ErrorMsg);
        m_connected = false;
        return false;
    }
}

void MotorController::shutdown() {
    if (!m_connected || !m_node) {
        if (m_mgr)
            m_mgr->PortsClose();
        return;
    }

    printf("Shutting down motor...\n");
    try {
        m_node->Motion.MoveVelStart(0);

        // Brief wait for motor to ramp down
        double timeout = m_mgr->TimeStampMsec() + 5000;
        while (!m_node->Motion.VelocityAtTarget()) {
            if (m_mgr->TimeStampMsec() > timeout)
                break;
        }

        m_node->EnableReq(false);
    } catch (mnErr &e) {
        fprintf(stderr, "Shutdown error: err=0x%08x msg=%s\n",
                e.ErrorCode, e.ErrorMsg);
    }

    m_mgr->PortsClose();
    m_connected = false;
    printf("Motor shutdown complete.\n");
}

bool MotorController::reconnect() {
    printf("Attempting reconnect...\n");

    // Close any existing connection
    try {
        m_mgr->PortsClose();
    } catch (...) {}

    m_connected = false;
    m_node = nullptr;

    return init();
}
