# Coffee Roaster Controller

Web-controlled drum motor speed for a coffee roaster. A C++ daemon commands a Teknic ClearPath-SC motor via the sFoundation SDK, and a Flask web server provides a phone-friendly UI.

## Prerequisites

- Raspberry Pi 4 running Raspberry Pi OS
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

## Services

| Service | What it does |
|---|---|
| `coffee-roaster` | Motor control daemon (C++, Unix socket at `/run/coffee-roaster/motor.sock`) |
| `coffee-roaster-web` | Web UI (Flask on port 8080) |

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

To uninstall only the motor daemon and web app:
```bash
sudo make uninstall
```

To perform a complete clean-slate uninstall of all application components, libraries, and kernel drivers:
```bash
../install_clean_slate.sh --uninstall
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
