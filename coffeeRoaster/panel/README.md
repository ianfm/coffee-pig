# Hardware panel — bring-up & testing

Step-by-step to get the OLED + knob + buttons working on the Pi. Do the
**foreground test (steps 1–5) first** — it surfaces wiring/library errors on the
console. Only set up the service (step 6) once the foreground run works.

The panel is a client of the motor daemon, exactly like the web UI. It will run
and show "Daemon offline" even if `coffee-roaster` isn't up, so you can test the
screen and controls independently of the motor.

## 1. Wire it up (Pi powered off)

| Board | Pi (BCM) |
|---|---|
| VCC | 3V3 |
| SDA | GPIO2 |
| SCL | GPIO3 |
| TRA | GPIO23 |
| TRB | GPIO24 |
| Push | GPIO25 |
| Con | GPIO22 |
| Bak | GPIO27 |

## 2. Enable I²C and fix permissions

**Raspberry Pi OS:**

```bash
sudo raspi-config nonint do_i2c 0          # enable the I2C bus
```

**Ubuntu (no `raspi-config`)** — enable the bus in the firmware config and load
the dev node, then reboot:

```bash
sudo apt install -y i2c-tools python3-lgpio
# Ubuntu on Pi keeps config.txt under /boot/firmware
echo 'dtparam=i2c_arm=on' | sudo tee -a /boot/firmware/config.txt
echo 'i2c-dev'            | sudo tee /etc/modules-load.d/i2c.conf
sudo reboot
```

**Permissions (both):**

```bash
# whoever RUNS the panel needs access to the I2C bus and GPIO character device
sudo usermod -aG i2c,gpio,dialout "$USER"   # then log out/in (or reboot)
ls -l /dev/i2c-1 /dev/gpiochip*             # note the group each is owned by
```

> Raspberry Pi OS ships `i2c`/`gpio` groups with udev rules, and the default
> user is already in them. **On Ubuntu those groups may not exist or may not own
> the device nodes** — check the `ls -l` output. If a node is owned by `root`
> with no usable group, either add a udev rule granting your group access, or
> just run the foreground test in step 5 with `sudo` to confirm the hardware
> before wiring up the service. The service user (step 6) still needs real group
> access — `sudo` is only a bring-up shortcut.

## 3. Confirm the display is on the bus

```bash
i2cdetect -y 1
```

Expect a device at **`3c`** (some panels are `3d`). If you see nothing, recheck
SDA/SCL/VCC/GND. Note the address — you may need it in step 5.

## 4. Install Python deps

```bash
cd coffeeRoaster/panel
pip3 install --break-system-packages -r requirements.txt
```

If `gpiozero` later complains it can't find a pin factory, install the lgpio
backend: `sudo apt install -y python3-lgpio`.

## 5. Foreground test

```bash
python3 panel.py
```

- A splash should appear, then the live readout (output RPM + status footer).
- Turn the knob: nothing changes yet (by design). **Push** the knob → "SET RPM"
  screen. Turn → value moves by 0.5. **Con** applies, **Bak** cancels; an idle
  edit auto-cancels after ~8 s.
- `Ctrl-C` to stop.

If the screen stays blank or throws an I/O error, edit [`config.py`](config.py):

| Symptom | Fix |
|---|---|
| `i2cdetect` showed `3d`, not `3c` | `I2C_ADDRESS = 0x3D` |
| Detected, but blank/garbled | flip `DISPLAY_CONTROLLER` between `"sh1106"` and `"ssd1306"` |
| Knob counts backwards | swap `PIN_ENCODER_A` / `PIN_ENCODER_B` (or the TRA/TRB wires) |
| A button does nothing | confirm its BCM pin and a solid ground |

To exercise the daemon link without the web UI:

```bash
echo GET     | socat - UNIX-CONNECT:/run/coffee-roaster/motor.sock
echo "SET 200" | socat - UNIX-CONNECT:/run/coffee-roaster/motor.sock   # 20.0 output RPM at ratio 10
```

The panel readout should track those values within ~250 ms.

## 6. Install as a service

Once foreground works:

```bash
cd ..              # back to coffeeRoaster/
make               # rebuilds the daemon too; needs the sFoundation SDK installed
sudo make install  # installs/refreshes all three services + copies panel to /opt
sudo systemctl enable --now coffee-roaster-panel
journalctl -u coffee-roaster-panel -f
```

`make install` sets all services to run as your user (via `set-user.sh`). That
user must be in the `i2c`/`gpio` groups from step 2, and must be the **same**
user the daemon runs as so it can reach the socket (`/run/.../motor.sock`, mode
0660).

## Common gotchas

- **"Daemon offline" on screen** — `coffee-roaster` isn't running, or the panel
  runs as a different user than owns the socket. Check
  `systemctl status coffee-roaster` and that both services share one `User=`.
- **Service fails, foreground works** — almost always the group/permission gap:
  `sudo usermod -aG i2c,gpio <svc-user>` and restart.
- **Gear ratio** — the readout is output-shaft RPM (`motorRpm / GEAR_RATIO`,
  default 10 in `config.py`). Set it to match your web UI's ratio.
