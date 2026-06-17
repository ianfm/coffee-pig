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

# TrueType fonts (the default PIL bitmap font is ~8px and unreadable across the
# room). First existing path wins; falls back to PIL's default if none found.
FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
]
FONT_SIZE_BIG = 36     # the RPM number (fills most of a 64px-tall panel)
FONT_SIZE_SMALL = 12   # headers / status footer

# ---------------------------------------------------------------------------
# Control behaviour
# ---------------------------------------------------------------------------
RPM_PER_DETENT = 5.0    # output RPM change per physical click of the knob
# Many detented encoders emit more than one quadrature step per physical click.
# If one click moves the value by N*5 RPM, set this to N (commonly 1, 2, or 4).
ENCODER_STEPS_PER_DETENT = 1
UI_TICK_S = 0.02        # render/encoder poll period (~50 Hz); render is coalesced
POLL_INTERVAL_S = 0.25  # how often the panel polls daemon status (~4 Hz)
EDIT_TIMEOUT_S = 8.0    # auto-cancel an edit after this much inactivity
BUTTON_BOUNCE_S = 0.05  # debounce window for the push buttons


def max_output_rpm() -> float:
    return MAX_MOTOR_RPM / GEAR_RATIO
