# Coffee Pig

Automated setup, control, and diagnostics for a coffee roaster drum motor using Teknic ClearPath-SC motors.

## Setup & Installation

Run the clean-slate install script to build the sFoundation SDK, the Exar USB driver, the motor daemon, and the Flask web service:

```bash
./install_clean_slate.sh
```

## Uninstallation

To remove all binaries, system services, SDK libraries, and kernel drivers:

```bash
./install_clean_slate.sh --uninstall
```

## Architecture

* **Daemon (`coffee-roaster`)**: A C++ service that communicates directly with the motor using the sFoundation SDK.
* **Web App (`coffee-roaster-web`)**: A Flask-based web dashboard (accessible at `http://localhost:8080`) that commands the daemon over a Unix domain socket.
