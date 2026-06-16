#!/usr/bin/env python3
"""
Coffee Roaster Web Controller

Flask app that provides a phone-friendly UI for controlling the drum motor.
Communicates with the C++ motor daemon via a Unix domain socket.

IPC Protocol (see socket_server.h):
  Send: "SET <rpm>\n" | "GET\n" | "STOP\n"
  Recv: "OK <targetRpm> <commandedRpm> <connected> <alert>\n"
      | "ERR <message>\n"
"""

import json
import os
import time
from flask import Flask, render_template, request, jsonify, Response

# Shared with the hardware panel; installed alongside server.py (see Makefile).
from ipc import daemon_command

app = Flask(__name__)

PRESETS_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "presets.json")


# ---------------------------------------------------------------------------
# Presets
# ---------------------------------------------------------------------------

def load_presets() -> list:
    if os.path.exists(PRESETS_FILE):
        with open(PRESETS_FILE) as f:
            return json.load(f)
    return []


def save_presets(presets: list):
    with open(PRESETS_FILE, "w") as f:
        json.dump(presets, f, indent=2)


# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------

@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/status")
def api_status():
    return jsonify(daemon_command("GET"))


@app.route("/api/speed", methods=["POST"])
def api_speed():
    rpm = request.json.get("rpm", 0)
    return jsonify(daemon_command(f"SET {rpm}"))


@app.route("/api/stop", methods=["POST"])
def api_stop():
    return jsonify(daemon_command("STOP"))


@app.route("/api/presets", methods=["GET"])
def api_presets_list():
    return jsonify(load_presets())


@app.route("/api/presets", methods=["POST"])
def api_presets_save():
    preset = request.json
    if not preset or "name" not in preset or "rpm" not in preset:
        return jsonify({"error": "need name and rpm"}), 400
    presets = load_presets()
    # Update existing or append
    for i, p in enumerate(presets):
        if p["name"] == preset["name"]:
            presets[i] = preset
            save_presets(presets)
            return jsonify({"ok": True})
    presets.append(preset)
    save_presets(presets)
    return jsonify({"ok": True})


@app.route("/api/presets/<name>", methods=["DELETE"])
def api_presets_delete(name):
    presets = [p for p in load_presets() if p["name"] != name]
    save_presets(presets)
    return jsonify({"ok": True})


@app.route("/api/status/stream")
def api_status_stream():
    """Server-Sent Events endpoint for live status updates."""
    def generate():
        while True:
            status = daemon_command("GET")
            yield f"data: {json.dumps(status)}\n\n"
            time.sleep(1)
    return Response(generate(), mimetype="text/event-stream",
                    headers={"Cache-Control": "no-cache",
                             "X-Accel-Buffering": "no"})


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080, threaded=True)
