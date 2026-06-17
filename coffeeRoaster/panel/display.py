"""
OLED rendering for the coffee-roaster panel.

Uses luma.oled to draw two screens that mirror what the web UI shows:
  - View:  live output RPM + daemon/motor/alert status
  - Edit:  the pending output RPM being dialled in, with control hints

The RPM number uses a large TrueType font so it's readable across the room;
labels use a small font. PIL text anchors handle centering.
"""

from luma.core.render import canvas
from PIL import ImageFont

import config

W = config.DISPLAY_WIDTH
H = config.DISPLAY_HEIGHT


def make_device():
    """Create the luma OLED device from config. Imported lazily so the module
    can be loaded for unit tests without I2C hardware present."""
    from luma.core.interface.serial import i2c

    serial = i2c(port=config.I2C_PORT, address=config.I2C_ADDRESS)
    if config.DISPLAY_CONTROLLER == "ssd1306":
        from luma.oled.device import ssd1306
        return ssd1306(serial, width=W, height=H)
    from luma.oled.device import sh1106
    return sh1106(serial, width=W, height=H)


def _load_font(size):
    for path in config.FONT_CANDIDATES:
        try:
            return ImageFont.truetype(path, size)
        except OSError:
            continue
    try:
        return ImageFont.load_default(size=size)  # Pillow >= 10.1
    except TypeError:
        return ImageFont.load_default()


class Display:
    def __init__(self, device):
        self.device = device
        self.font_big = _load_font(config.FONT_SIZE_BIG)
        self.font = _load_font(config.FONT_SIZE_SMALL)

    # -- public API ---------------------------------------------------------

    def show_splash(self):
        with canvas(self.device) as draw:
            draw.text((W // 2, H // 2), "COFFEE ROASTER",
                      font=self.font, fill="white", anchor="mm")

    def show_view(self, status: dict):
        """status is the dict returned by ipc.get_status()."""
        with canvas(self.device) as draw:
            if not status.get("ok"):
                draw.text((W // 2, H // 2 - 8), "DAEMON",
                          font=self.font, fill="white", anchor="mm")
                draw.text((W // 2, H // 2 + 8), "offline",
                          font=self.font, fill="white", anchor="mm")
                return

            commanded_out = status["commandedRpm"] / config.GEAR_RATIO
            target_out = status["targetRpm"] / config.GEAR_RATIO

            # If the motor hasn't reached the setpoint yet, show the target small.
            if abs(target_out - commanded_out) > 0.05:
                draw.text((2, 0), f"→ {target_out:.0f}",
                          font=self.font, fill="white", anchor="lt")

            # Big current (commanded) output RPM, centred.
            draw.text((W // 2, 26), f"{commanded_out:.1f}",
                      font=self.font_big, fill="white", anchor="mm")

            draw.text((W // 2, H), _status_word(status),
                      font=self.font, fill="white", anchor="mb")

    def show_edit(self, pending_output_rpm: float):
        with canvas(self.device) as draw:
            draw.text((W // 2, 0), "SET RPM",
                      font=self.font, fill="white", anchor="mt")
            draw.text((W // 2, 32), f"{pending_output_rpm:.0f}",
                      font=self.font_big, fill="white", anchor="mm")
            draw.text((W // 2, H), "turn  ✓ set  ✕ back",
                      font=self.font, fill="white", anchor="mb")


def _status_word(status: dict) -> str:
    if status["hasAlert"]:
        return "ALERT"
    if not status["motorConnected"]:
        return "NO MOTOR"
    return "RUN" if status["motorEnabled"] else "IDLE"
