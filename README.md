# Coffee Pig

Automated setup, control daemon, and web dashboard for regulating coffee roaster drum speed using a Teknic ClearPath-SC motor via the SC4-Hub.

---

## Architecture Overview

The system consists of three main components:

1. **Kernel Driver (`Teknic_SC4Hub_USB_Driver`)**: 
   Compiles and binds the Exar USB-to-serial driver (`xr_usb_serial_common`) to map the SC4-Hub to `/dev/ttyXRUSB*`.
2. **C++ Daemon (`coffee-roaster`)**: 
   Interfaces with the motor using the sFoundation SDK. It handles the high-frequency control loop (20 Hz), tracks motor state, auto-recovers from disconnects, and opens a Unix Domain Socket for IPC.
3. **Web Dashboard (`coffee-roaster-web`)**: 
   A lightweight Flask application serving a web UI (bound to port `8080`) that sends commands to the C++ daemon via Unix socket IPC.

```
                  +--------------------------------+
                  |         Web Dashboard          |
                  |     (Flask Web App on :8080)   |
                  +--------------------------------+
                                  | (Unix Socket IPC)
                                  v
                  +--------------------------------+
                  |         coffee-roaster         |
                  |          (C++ Daemon)          |
                  +--------------------------------+
                                  | (sFoundation SDK)
                                  v
                  +--------------------------------+
                  |         SC4-Hub Driver         |
                  |     (xr_usb_serial_common)     |
                  +--------------------------------+
                                  |
                                  v
                         [ClearPath-SC Motor]
```

---

## Getting Started

### 1. Hardware Setup
1. Connect the ClearPath-SC motor to the coffee roaster drum.
2. Connect the SC4-Hub via USB to the host computer (e.g. Raspberry Pi).
3. Ensure the motor and hub are powered.

### 2. Automatic Installation
Run the setup script from the root of the repository to install prerequisites, build all components (SDK, driver, daemon), and register systemd services:
```bash
./setup.sh
```
*Note: The script will prompt for your `sudo` password to install system dependencies, copy libraries to `/usr/local/lib/`, load the kernel module, and register the services.*

### 3. Verification & Access
Access the web dashboard in your browser at:
```
http://localhost:8080
```

---

## Service Administration

Both the control daemon and the web portal run as systemd services. Use the following commands to administer them:

### Managing Services
```bash
# Check status of both services
systemctl status coffee-roaster coffee-roaster-web

# Restart services (after recompiling or hardware resets)
sudo systemctl restart coffee-roaster coffee-roaster-web

# Stop services
sudo systemctl stop coffee-roaster coffee-roaster-web

# Disable from auto-starting on system boot
sudo systemctl disable coffee-roaster coffee-roaster-web
```

### Monitoring Logs
```bash
# View real-time logs for the motor control daemon
journalctl -u coffee-roaster -f

# View real-time logs for the web application
journalctl -u coffee-roaster-web -f
```

---

## Uninstallation

To cleanly remove all binaries, registered systemd services, system libraries, and the kernel driver:
```bash
./setup.sh --uninstall
```

---

## Developer Guide

### 1. Configuration
Core drum motor and loop parameters are located in `coffeeRoaster/config.h`:
* `MAX_RPM`: Velocity limit for safety (default: `1000.0` RPM motor shaft, yielding `100` RPM drum speed at 10:1 gearing).
* `ACC_LIM_RPM_PER_SEC`: Acceleration limit, set low (default: `100.0`) for gentle ramp-up of a loaded drum.
* `LOOP_PERIOD_US`: Control loop frequency (default: `50000` µs = 20 Hz).
* `SOCKET_PATH`: Path to Unix socket (default: `/run/coffee-roaster/motor.sock`).

To apply changes, rebuild the binary and restart the service:
```bash
cd coffeeRoaster
make real_clean && make
sudo make install
sudo systemctl restart coffee-roaster
```

### 2. Unix Domain Socket IPC Protocol
The C++ daemon listens on `/run/coffee-roaster/motor.sock`. You can inspect or command the daemon directly using `socat`:

#### IPC Commands Reference

| Command | Argument | Response Format | Description |
|---|---|---|---|
| `GET` | *None* | `OK <target_rpm> <commanded_rpm> <connected> <has_alert> <enabled>` | Returns basic connection and motor state. |
| `SET` | `<rpm>` | `OK <target_rpm> <commanded_rpm> <connected> <has_alert> <enabled>` | Sets target velocity and enables the motor. |
| `STOP` | *None* | `OK <target_rpm> <commanded_rpm> <connected> <has_alert> <enabled>` | Decelerates motor to 0 RPM. |

#### Manual Testing Examples:
```bash
# Query daemon status
echo "GET" | socat - UNIX-CONNECT:/run/coffee-roaster/motor.sock

# Set motor speed to 150 RPM
echo "SET 150" | socat - UNIX-CONNECT:/run/coffee-roaster/motor.sock

# Stop the roaster drum
echo "STOP" | socat - UNIX-CONNECT:/run/coffee-roaster/motor.sock
```
