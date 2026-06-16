#!/usr/bin/env python3
"""
Coffee Roaster hardware panel.

A standalone client of the motor daemon (exactly like the Flask web server):
it polls status over the Unix socket and renders it to an I2C OLED, and lets
the operator dial in a new RPM with a rotary encoder + Confirm/Back buttons.

Controls:
  - Push the knob       -> enter EDIT mode (seeded from the current target)
  - Turn the knob        -> adjust the pending RPM (EDIT mode only)
  - Confirm button       -> commit the pending RPM, return to VIEW
  - Back button          -> discard the edit, return to VIEW

The daemon is the single hardware authority and serializes every command, so
running this alongside the web UI is safe with no extra locking on its side.
"""

import signal
import threading
import time

from gpiozero import RotaryEncoder, Button

import config
import display
import ipc

VIEW = "view"
EDIT = "edit"


class Panel:
    def __init__(self, disp: display.Display):
        self.disp = disp
        self.lock = threading.Lock()
        self.mode = VIEW
        self.status = {"ok": False}
        self.pending_output_rpm = 0.0
        self.last_input_ts = 0.0
        self._running = True

    # -- rendering ----------------------------------------------------------

    def _render_locked(self):
        """Draw the screen for the current state. Caller must hold self.lock."""
        if self.mode == EDIT:
            self.disp.show_edit(self.pending_output_rpm)
        else:
            self.disp.show_view(self.status)

    def render(self):
        with self.lock:
            self._render_locked()

    # -- input callbacks (run on gpiozero's own threads) --------------------

    def on_rotate(self, encoder: RotaryEncoder):
        with self.lock:
            if self.mode != EDIT:
                return
            step = encoder.steps  # signed accumulated detents since last reset
            encoder.steps = 0
            self.pending_output_rpm = _clamp(
                self.pending_output_rpm + step * config.RPM_STEP,
                0.0, config.max_output_rpm())
            self.last_input_ts = time.monotonic()
            self._render_locked()

    def on_push(self):
        """Enter edit mode, seeded from the current target."""
        with self.lock:
            if self.mode == EDIT:
                return
            seed = 0.0
            if self.status.get("ok"):
                seed = self.status["targetRpm"] / config.GEAR_RATIO
            self.pending_output_rpm = _clamp(seed, 0.0, config.max_output_rpm())
            self.mode = EDIT
            self.last_input_ts = time.monotonic()
            self._render_locked()

    def on_confirm(self):
        with self.lock:
            if self.mode != EDIT:
                return
            motor_rpm = round(self.pending_output_rpm * config.GEAR_RATIO, 1)
            self.mode = VIEW
        # Talk to the daemon outside the lock (network I/O may block briefly).
        result = ipc.set_motor_rpm(motor_rpm)
        with self.lock:
            if result.get("ok"):
                self.status = result
            self._render_locked()

    def on_back(self):
        with self.lock:
            if self.mode != EDIT:
                return
            self.mode = VIEW
            self._render_locked()

    # -- background status poller -------------------------------------------

    def poll_loop(self):
        while self._running:
            status = ipc.get_status()
            with self.lock:
                self.status = status
                if self.mode == VIEW:
                    self._render_locked()
                elif time.monotonic() - self.last_input_ts > config.EDIT_TIMEOUT_S:
                    # Auto-cancel a stale edit (e.g. a bumped knob).
                    self.mode = VIEW
                    self._render_locked()
            time.sleep(config.POLL_INTERVAL_S)

    def stop(self):
        self._running = False


def _clamp(value, lo, hi):
    return max(lo, min(hi, value))


def main():
    disp = display.Display(display.make_device())
    disp.show_splash()

    panel = Panel(disp)

    # Rotary encoder: accumulate steps, react on rotation.
    encoder = RotaryEncoder(config.PIN_ENCODER_A, config.PIN_ENCODER_B,
                            max_steps=0)
    encoder.when_rotated = lambda: panel.on_rotate(encoder)

    push = Button(config.PIN_ENCODER_PUSH, bounce_time=config.BUTTON_BOUNCE_S)
    confirm = Button(config.PIN_CONFIRM, bounce_time=config.BUTTON_BOUNCE_S)
    back = Button(config.PIN_BACK, bounce_time=config.BUTTON_BOUNCE_S)
    push.when_pressed = panel.on_push
    confirm.when_pressed = panel.on_confirm
    back.when_pressed = panel.on_back

    poller = threading.Thread(target=panel.poll_loop, daemon=True)
    poller.start()

    stop_event = threading.Event()
    signal.signal(signal.SIGTERM, lambda *_: stop_event.set())
    signal.signal(signal.SIGINT, lambda *_: stop_event.set())
    stop_event.wait()

    panel.stop()


if __name__ == "__main__":
    main()
