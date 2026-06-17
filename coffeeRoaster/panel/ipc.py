"""
IPC client for the C++ motor daemon.

Shared by the Flask web server and the hardware panel. Speaks the Unix-socket
text protocol defined in ../socket_server.h:

  Send: "SET <rpm>\n" | "GET\n" | "STOP\n"
  Recv: "OK <targetRpm> <commandedRpm> <connected> <alert> <enabled>\n"
      | "ERR <message>\n"
"""

import socket

SOCKET_PATH = "/run/coffee-roaster/motor.sock"


def daemon_command(cmd: str, timeout: float = 2.0) -> dict:
    """Send a command to the motor daemon and parse the response."""
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        sock.connect(SOCKET_PATH)
        sock.sendall((cmd + "\n").encode())
        data = sock.recv(256).decode().strip()
        sock.close()
    except (socket.error, OSError) as e:
        return {"ok": False, "error": f"Daemon unreachable: {e}"}

    if data.startswith("OK "):
        parts = data[3:].split()
        if len(parts) >= 5:
            return {
                "ok": True,
                "targetRpm": float(parts[0]),
                "commandedRpm": float(parts[1]),
                "motorConnected": parts[2] == "1",
                "hasAlert": parts[3] == "1",
                "motorEnabled": parts[4] == "1",
            }
        elif len(parts) >= 4:
            return {
                "ok": True,
                "targetRpm": float(parts[0]),
                "commandedRpm": float(parts[1]),
                "motorConnected": parts[2] == "1",
                "hasAlert": parts[3] == "1",
                "motorEnabled": True,
            }
        return {"ok": True, "raw": data}
    elif data.startswith("ERR "):
        return {"ok": False, "error": data[4:]}
    else:
        return {"ok": False, "error": f"Unexpected response: {data}"}


# ---------------------------------------------------------------------------
# Convenience helpers (used by the hardware panel)
# ---------------------------------------------------------------------------

def get_status() -> dict:
    return daemon_command("GET")


def set_motor_rpm(rpm: float) -> dict:
    return daemon_command(f"SET {rpm}")


def stop() -> dict:
    return daemon_command("STOP")
