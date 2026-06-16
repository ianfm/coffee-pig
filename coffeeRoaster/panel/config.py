"""
Configuration for the coffee-roaster hardware panel.

All tunables live here: GPIO pin assignments (BCM numbering), display
parameters, and control behaviour. Edit and restart the panel service to apply.
"""

# ---------------------------------------------------------------------------
# IPC - must match the motor daemon (see ../config.h and ../socket_server.cpp)
# ---------------------------------------------------------------------------
SOCKET_PATH = "/run/coffee-roaster/motor.sock"
MAX_MOTOR_RPM = 1000.0  # mirrors MAX_RPM in ../config.h

# ---------------------------------------------------------------------------
# Gear ratio
# ---------------------------------------------------------------------------
# The daemon speaks motor-shaft RPM; the panel (like the web UI) shows and
# adjusts OUTPUT-shaft RPM:  outputRpm = motorRpm / GEAR_RATIO.
# NOTE: the web UI stores its own gear ratio in browser localStorage and cannot
# share it with this process, so keep this value in sync with the web setting.
GEAR_RATIO = 10.0

# ---------------------------------------------------------------------------
# GPIO pins (BCM numbering) - from the wiring diagram
# ---------------------------------------------------------------------------
PIN_ENCODER_A = 23   # TRA
PIN_ENCODER_B = 24   # TRB
PIN_ENCODER_PUSH = 25  # Push  -> enter edit mode
PIN_CONFIRM = 22     # Con    -> commit the pending RPM
PIN_BACK = 27        # Bak    -> cancel the edit
# I2C SDA=GPIO2, SCL=GPIO3 are handled by the kernel I2C driver (i2c-1).

# ---------------------------------------------------------------------------
# OLED display (I2C)
# ---------------------------------------------------------------------------
I2C_PORT = 1
I2C_ADDRESS = 0x3C
DISPLAY_CONTROLLER = "sh1106"  # "sh1106" (1.3") or "ssd1306" (0.96")
DISPLAY_WIDTH = 128
DISPLAY_HEIGHT = 64

# ---------------------------------------------------------------------------
# Control behaviour
# ---------------------------------------------------------------------------
RPM_STEP = 0.5          # output RPM change per encoder detent
POLL_INTERVAL_S = 0.25  # how often the panel polls daemon status (~4 Hz)
EDIT_TIMEOUT_S = 8.0    # auto-cancel an edit after this much inactivity
BUTTON_BOUNCE_S = 0.05  # debounce window for the push buttons


def max_output_rpm() -> float:
    return MAX_MOTOR_RPM / GEAR_RATIO
