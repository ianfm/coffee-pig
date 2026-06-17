# Coffee Roaster Controller

Web-controlled drum motor speed for a coffee roaster. A C++ daemon commands a Teknic ClearPath-SC motor via the sFoundation SDK, and a Flask web server provides a phone-friendly UI. An optional hardware panel (OLED + rotary knob + buttons) offers the same readout and speed control without a phone.

## Architecture

The C++ daemon is the single motor authority. It exposes a tiny text protocol over a Unix socket (`/run/coffee-roaster/motor.sock`). Everything else is a **client** of that socket, so clients can run side by side and the daemon serializes their commands:

```
                       Unix socket (GET / SET <rpm> / STOP)
  coffee-roaster  <-------------------------------------------+--- coffee-roaster-web   (Flask UI)
  (C++ motor daemon)                                          +--- coffee-roaster-panel (OLED + knob)
```

## Prerequisites

- Raspberry Pi 4B running Ubuntu 24.04 (64-bit)
- Teknic ClearPath-SC motor + SC4-HUB connected via USB
- Python 3 with pip
- User in the `dialout` group (`sudo usermod -aG dialout $USER`, then log out/in)
- sFoundation SDK built and installed as a shared library — follow **all steps** in [`../sFoundation/readme.txt`](../sFoundation/readme.txt) (serial port permissions, build, systemwide install, and SC4-Hub USB driver)

## Build & Install

```bash
cd coffeeRoaster
make
sudo make install
pip3 install --break-system-packages flask
sudo systemctl enable --now coffee-roaster coffee-roaster-web
```

`make install` will automatically set the service files to run as the current user if not already configured.

## Usage

Open `http://<pi-ip>:8080` on your phone.

## Hardware panel (optional)

An I²C OLED, a rotary encoder with a push knob, and Confirm/Back buttons give a phone-free readout and speed control.

**Wiring (BCM GPIO):**

| Signal | Pi pin (BCM) |
|---|---|
| VCC | 3V3 |
| SDA | GPIO2 |
| SCL | GPIO3 |
| Encoder A (TRA) | GPIO23 |
| Encoder B (TRB) | GPIO24 |
| Encoder push | GPIO25 |
| Confirm | GPIO22 |
| Back | GPIO27 |

**Controls:** push the knob to enter edit mode, turn it to dial in an RPM, **Confirm** to apply or **Back** to cancel (an idle edit auto-cancels). The readout shows output-shaft RPM, just like the web UI.

**Bring-up & testing:** see [panel/README.md](panel/README.md) for the full
step-by-step (wiring, enabling I²C on **Raspberry Pi OS or Ubuntu**, permissions,
a foreground test, and the systemd service). Quick version once I²C is enabled:

```bash
pip3 install --break-system-packages -r panel/requirements.txt
sudo systemctl enable --now coffee-roaster-panel
```

Verify the OLED is detected with `i2cdetect -y 1` (expect `0x3c`). Pin assignments, the gear ratio, and the OLED controller (`sh1106` for 1.3", `ssd1306` for 0.96") are set in [panel/config.py](panel/config.py). Keep `GEAR_RATIO` in sync with the web UI's gear-ratio setting (the web stores its ratio in the browser, so the two can't share it automatically).

## Services

| Service | What it does |
|---|---|
| `coffee-roaster` | Motor control daemon (C++, Unix socket at `/run/coffee-roaster/motor.sock`) |
| `coffee-roaster-web` | Web UI (Flask on port 8080) |
| `coffee-roaster-panel` | Hardware OLED + knob + buttons (Python) |

```bash
# Check status
systemctl status coffee-roaster coffee-roaster-web

# View logs
journalctl -u coffee-roaster -f
journalctl -u coffee-roaster-web -f

# Restart
sudo systemctl restart coffee-roaster coffee-roaster-web
```

## Uninstall

```bash
sudo make uninstall
```

## Configuration

Edit [config.h](config.h) and rebuild. Key parameters:

| Constant | Default | Description |
|---|---|---|
| `MAX_RPM` | 60 | Maximum settable speed |
| `ACC_LIM_RPM_PER_SEC` | 100 | Acceleration limit |

## Debugging

Talk to the daemon directly without the web server:

```bash
echo "GET" | socat - UNIX-CONNECT:/run/coffee-roaster/motor.sock
echo "SET 15" | socat - UNIX-CONNECT:/run/coffee-roaster/motor.sock
echo "STOP" | socat - UNIX-CONNECT:/run/coffee-roaster/motor.sock
```
