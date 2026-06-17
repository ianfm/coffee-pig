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

Design note: input handlers do *no* slow work. The rotary encoder just
accumulates counts in gpiozero; a single UI loop polls that count and redraws at
a fixed cadence, coalescing a fast spin into one update. Rendering an I2C OLED
takes tens of ms, so doing it per-detent (the old approach) built a multi-second
backlog -- hence the lag/overshoot. Keep render out of the callbacks.
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
    def __init__(self, disp: display.Display, encoder: RotaryEncoder):
        self.disp = disp
        self.encoder = encoder
        self.lock = threading.Lock()
        self.mode = VIEW
        self.status = {"ok": False}
        self.pending_output_rpm = 0.0
        self.last_input = 0.0
        self.last_steps = encoder.steps
        self.dirty = True
        self._running = True
        self._last_status_poll = 0.0

    # -- input callbacks (gpiozero threads): mutate state only, never render ---

    def on_push(self):
        with self.lock:
            if self.mode == EDIT:
                return
            seed = 0.0
            if self.status.get("ok"):
                seed = self.status["targetRpm"] / config.GEAR_RATIO
            self.pending_output_rpm = _clamp(seed, 0.0, config.max_output_rpm())
            self.mode = EDIT
            self.last_input = time.monotonic()
            self.dirty = True

    def on_confirm(self):
        with self.lock:
            if self.mode != EDIT:
                return
            motor_rpm = round(self.pending_output_rpm * config.GEAR_RATIO, 1)
            self.mode = VIEW
            self.dirty = True
        # Network I/O outside the lock.
        result = ipc.set_motor_rpm(motor_rpm)
        with self.lock:
            if result.get("ok"):
                self.status = result
            self.dirty = True

    def on_back(self):
        with self.lock:
            if self.mode != EDIT:
                return
            self.mode = VIEW
            self.dirty = True

    # -- single UI loop: poll encoder + status, render at most once per tick ---

    def ui_loop(self):
        while self._running:
            now = time.monotonic()

            # Poll daemon status at its slower cadence (socket I/O, no lock held).
            if now - self._last_status_poll >= config.POLL_INTERVAL_S:
                self._last_status_poll = now
                status = ipc.get_status()
                with self.lock:
                    if status != self.status:
                        self.status = status
                        if self.mode == VIEW:
                            self.dirty = True

            # Apply accumulated encoder motion, handle edit timeout, decide render.
            with self.lock:
                steps = self.encoder.steps
                delta = steps - self.last_steps
                self.last_steps = steps

                if self.mode == EDIT:
                    if delta != 0:
                        rpm_delta = (delta / config.ENCODER_STEPS_PER_DETENT) \
                            * config.RPM_PER_DETENT
                        new_val = _clamp(self.pending_output_rpm + rpm_delta,
                                         0.0, config.max_output_rpm())
                        if new_val != self.pending_output_rpm:
                            self.pending_output_rpm = new_val
                            self.dirty = True
                        self.last_input = now
                    elif now - self.last_input > config.EDIT_TIMEOUT_S:
                        self.mode = VIEW
                        self.dirty = True

                render = self.dirty
                self.dirty = False
                mode = self.mode
                pending = self.pending_output_rpm
                status = self.status

            # Render outside the lock (I2C is slow).
            if render:
                if mode == EDIT:
                    self.disp.show_edit(pending)
                else:
                    self.disp.show_view(status)

            time.sleep(config.UI_TICK_S)

    def stop(self):
        self._running = False


def _clamp(value, lo, hi):
    return max(lo, min(hi, value))


def main():
    encoder = RotaryEncoder(config.PIN_ENCODER_A, config.PIN_ENCODER_B,
                            max_steps=0)

    disp = display.Display(display.make_device())
    disp.show_splash()

    panel = Panel(disp, encoder)

    push = Button(config.PIN_ENCODER_PUSH, bounce_time=config.BUTTON_BOUNCE_S)
    confirm = Button(config.PIN_CONFIRM, bounce_time=config.BUTTON_BOUNCE_S)
    back = Button(config.PIN_BACK, bounce_time=config.BUTTON_BOUNCE_S)
    push.when_pressed = panel.on_push
    confirm.when_pressed = panel.on_confirm
    back.when_pressed = panel.on_back

    signal.signal(signal.SIGTERM, lambda *_: panel.stop())
    signal.signal(signal.SIGINT, lambda *_: panel.stop())

    panel.ui_loop()


if __name__ == "__main__":
    main()
