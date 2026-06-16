"""
OLED rendering for the coffee-roaster panel.

Uses luma.oled to draw two screens that mirror what the web UI shows:
  - View:  live output RPM + daemon/motor/alert status
  - Edit:  the pending output RPM being dialled in, with control hints
"""

from luma.core.render import canvas
from PIL import ImageFont

import config


def make_device():
    """Create the luma OLED device from config. Imported lazily so the module
    can be loaded for unit tests without I2C hardware present."""
    from luma.core.interface.serial import i2c

    serial = i2c(port=config.I2C_PORT, address=config.I2C_ADDRESS)
    if config.DISPLAY_CONTROLLER == "ssd1306":
        from luma.oled.device import ssd1306
        return ssd1306(serial, width=config.DISPLAY_WIDTH, height=config.DISPLAY_HEIGHT)
    from luma.oled.device import sh1106
    return sh1106(serial, width=config.DISPLAY_WIDTH, height=config.DISPLAY_HEIGHT)


class Display:
    def __init__(self, device):
        self.device = device
        # Default bitmap fonts keep this dependency-free; sizes are nominal.
        self.font_big = ImageFont.load_default()
        self.font = ImageFont.load_default()

    # -- public API ---------------------------------------------------------

    def show_splash(self):
        with canvas(self.device) as draw:
            draw.text((20, 24), "COFFEE ROASTER", fill="white")

    def show_view(self, status: dict):
        """status is the dict returned by ipc.get_status()."""
        with canvas(self.device) as draw:
            if not status.get("ok"):
                draw.text((0, 0), "Daemon offline", fill="white")
                draw.text((0, 28), "no connection", fill="white")
                return

            commanded_out = status["commandedRpm"] / config.GEAR_RATIO
            target_out = status["targetRpm"] / config.GEAR_RATIO

            # Big current (commanded) output RPM
            draw.text((0, 2), f"{commanded_out:5.1f} RPM", fill="white")
            if abs(target_out - commanded_out) > 0.05:
                draw.text((0, 18), f"target {target_out:.1f}", fill="white")

            # Status footer
            footer = []
            footer.append("Motor" if status["motorConnected"] else "No motor")
            if status["hasAlert"]:
                footer.append("ALERT")
            elif status["motorEnabled"]:
                footer.append("run")
            else:
                footer.append("idle")
            draw.text((0, 50), "  ".join(footer), fill="white")

    def show_edit(self, pending_output_rpm: float):
        with canvas(self.device) as draw:
            draw.text((0, 0), "SET RPM", fill="white")
            draw.text((0, 20), f"{pending_output_rpm:5.1f}", fill="white")
            draw.text((0, 50), "turn  OK set  X back", fill="white")
