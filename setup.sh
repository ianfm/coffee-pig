#!/bin/bash
# setup.sh
#
# Automated installation / uninstallation script for a clean-slate setup of the coffee-pig repo.
# Run this script to build & install the sFoundation SDK, the Exar USB driver,
# the motor daemon, and the Flask web service.
#
# Usage: ./setup.sh [-u|--uninstall]

set -euo pipefail

# Output colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}==================================================================${NC}"
echo -e "${BLUE}          Coffee Pig Repository Clean-Slate Installer             ${NC}"
echo -e "${BLUE}==================================================================${NC}"

# Check if script is run as root
if [[ ${EUID:-0} -eq 0 ]]; then
    echo -e "${RED}[ERROR]: Please do not run this script as root/sudo directly.${NC}"
    echo -e "The script will request sudo authorization when needed, preserving your user configuration."
    exit 1
fi

UNINSTALL=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -u|--uninstall)
            UNINSTALL=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [-u|--uninstall]"
            exit 1
            ;;
    esac
done

if [ "$UNINSTALL" = true ]; then
    echo -e "${YELLOW}Uninstalling Coffee Pig application and services...${NC}"

    # Stop and disable services
    echo "Stopping and disabling systemd services..."
    sudo systemctl stop coffee-roaster coffee-roaster-web || true
    sudo systemctl disable coffee-roaster coffee-roaster-web || true

    # Remove systemd service files
    echo "Removing systemd service files..."
    sudo rm -f /etc/systemd/system/coffee-roaster.service
    sudo rm -f /etc/systemd/system/coffee-roaster-web.service
    sudo systemctl daemon-reload

    # Remove installed binaries and files
    echo "Removing application binaries and files..."
    sudo rm -f /usr/local/bin/coffee-roaster
    sudo rm -rf /opt/coffee-roaster

    # Remove sFoundation libraries
    echo "Removing sFoundation SDK libraries..."
    sudo rm -f /usr/local/lib/libsFoundation20.so
    sudo rm -f /usr/local/lib/MNuserDriver20.xml
    sudo ldconfig

    # Unload and remove SC4-Hub USB Driver
    echo "Unloading and removing SC4-Hub USB Driver..."
    sudo modprobe -r xr_usb_serial_common || true
    sudo rm -f /lib/modules/"$(uname -r)"/kernel/drivers/usb/serial/xr_usb_serial_common.ko
    sudo depmod -a

    # Remove module auto-load configuration
    if [ -f /etc/modules ]; then
        sudo sed -i '/xr_usb_serial_common/d' /etc/modules
    fi

    echo -e "${GREEN}Uninstall completed successfully!${NC}"
    exit 0
fi

CURRENT_USER=$(whoami)
REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_ROOT"

# [1/6] Check prerequisites incrementally
echo -e "${GREEN}[1/6] Checking system prerequisites...${NC}"
MISSING_PKGS=()
for pkg in build-essential linux-headers-$(uname -r) python3-pip python3-flask socat; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        MISSING_PKGS+=("$pkg")
    fi
done

if [ ${#MISSING_PKGS[@]} -gt 0 ]; then
    echo -e "${YELLOW}Installing missing prerequisites: ${MISSING_PKGS[*]}...${NC}"
    sudo apt update
    sudo apt install -y "${MISSING_PKGS[@]}"
else
    echo "All system prerequisites are already installed."
fi

echo -e "${GREEN}[2/6] Setting up serial port groups...${NC}"
if groups "$CURRENT_USER" | grep -q "\bdialout\b"; then
    echo "User is already in the 'dialout' group."
else
    echo "Adding $CURRENT_USER to the 'dialout' group..."
    sudo usermod -aG dialout "$CURRENT_USER"
    echo -e "${YELLOW}[NOTE]: You will need to log out and log back in (or reboot) for group changes to take effect.${NC}"
fi

# [3/6] Build sFoundation SDK incrementally
echo -e "${GREEN}[3/6] Building sFoundation SDK (incremental)...${NC}"
cd sFoundation/sFoundation
make # Incremental build
INSTALLED_LIB="/usr/local/lib/libsFoundation20.so"
if [ ! -f "$INSTALLED_LIB" ] || [ libsFoundation20.so -nt "$INSTALLED_LIB" ]; then
    echo "Installing libsFoundation20.so systemwide..."
    sudo cp libsFoundation20.so /usr/local/lib/
    sudo cp MNuserDriver20.xml /usr/local/lib/
    sudo ldconfig
fi

# Verify sFoundation installation
if ldconfig -p | grep -q "libsFoundation20"; then
    echo -e "${GREEN}sFoundation SDK installed and registered successfully.${NC}"
else
    echo -e "${YELLOW}Warning: ldconfig could not locate libsFoundation20.so. Adding /usr/local/lib to ld search path...${NC}"
    echo "/usr/local/lib" | sudo tee -a /etc/ld.so.conf.d/libc.conf
    sudo ldconfig
fi

# [4/6] Build driver incrementally
echo -e "${GREEN}[4/6] Building Teknic SC4-Hub USB Driver (incremental)...${NC}"
cd "$REPO_ROOT"/Teknic_SC4Hub_USB_Driver/ExarKernelDriver
make # Incremental build
INSTALLED_KO="/lib/modules/$(uname -r)/kernel/drivers/usb/serial/xr_usb_serial_common.ko"
if [ ! -f "$INSTALLED_KO" ] || [ xr_usb_serial_common.ko -nt "$INSTALLED_KO" ] || ! lsmod | grep -q "xr_usb_serial_common"; then
    echo "Installing Exar USB serial module..."
    sudo mkdir -p "$(dirname "$INSTALLED_KO")"
    sudo cp xr_usb_serial_common.ko "$INSTALLED_KO"
    sudo depmod -a

    if ! grep -q "xr_usb_serial_common" /etc/modules; then
        echo "xr_usb_serial_common" | sudo tee -a /etc/modules
    fi

    echo "Loading module..."
    sudo modprobe -r xr_usb_serial_common || true
    sudo modprobe xr_usb_serial_common || sudo insmod "$INSTALLED_KO"
fi

# Run driver binding check
echo "Binding SC4-Hub to Exar USB Serial driver..."
BUS_ID=""
if grep -q v2890p0213 /sys/bus/usb/devices/*/modalias; then
    BUS_ID=$(grep v2890p0213 /sys/bus/usb/devices/*/modalias | head -1 | sed 's/\/sys\/bus\/usb\/devices\///g;s/\/.*//g')
    if [ -d "/sys/bus/usb/drivers/cdc_acm/$BUS_ID" ] ; then
        echo "Unbinding device from default cdc_acm driver..."
        echo -n "$BUS_ID" | sudo tee /sys/bus/usb/drivers/cdc_acm/unbind > /dev/null || true
    fi
    if [ ! -d "/sys/bus/usb/drivers/cdc_xr_usb_serial/$BUS_ID" ] ; then
        echo "Binding device to Exar driver..."
        echo -n "$BUS_ID" | sudo tee /sys/bus/usb/drivers/cdc_xr_usb_serial/bind > /dev/null || true
    fi
    echo -e "${GREEN}SC4-Hub successfully bound to cdc_xr_usb_serial driver.${NC}"
else
    echo -e "${YELLOW}[NOTE]: SC4-Hub USB device not found at the moment. Driver module loaded and configured for next plug-in.${NC}"
fi

# [5/6] Compile and install Coffee Roaster Daemon
echo -e "${GREEN}[5/6] Compiling Coffee Roaster Daemon (incremental)...${NC}"
cd "$REPO_ROOT"/coffeeRoaster
make # Incremental build

echo -e "${GREEN}Running tests...${NC}"
make test

OLD_BIN_MD5=""
OLD_WEB_MD5=""
if [ -f /usr/local/bin/coffee-roaster ]; then
    OLD_BIN_MD5=$(md5sum /usr/local/bin/coffee-roaster | awk '{print $1}')
fi
if [ -f /opt/coffee-roaster/web/templates/index.html ]; then
    OLD_WEB_MD5=$(md5sum /opt/coffee-roaster/web/templates/index.html | awk '{print $1}')
fi

echo "Installing services..."
sudo make install

NEW_BIN_MD5=$(md5sum /usr/local/bin/coffee-roaster | awk '{print $1}')
NEW_WEB_MD5=$(md5sum /opt/coffee-roaster/web/templates/index.html | awk '{print $1}')

# [6/6] Start/restart services
if [ "$OLD_BIN_MD5" != "$NEW_BIN_MD5" ] || [ "$OLD_WEB_MD5" != "$NEW_WEB_MD5" ] || ! systemctl is-active --quiet coffee-roaster || ! systemctl is-active --quiet coffee-roaster-web; then
    echo -e "${GREEN}[6/6] Starting/restarting services...${NC}"
    sudo systemctl daemon-reload
    sudo systemctl enable coffee-roaster coffee-roaster-web
    sudo systemctl restart coffee-roaster coffee-roaster-web
else
    echo -e "${GREEN}[6/6] Services already running and up-to-date.${NC}"
fi

echo -e "${BLUE}==================================================================${NC}"
echo -e "${GREEN}                Installation Completed Successfully!              ${NC}"
echo -e "${BLUE}==================================================================${NC}"
echo -e "Please verify system services using:"
echo -e "  ${YELLOW}systemctl status coffee-roaster coffee-roaster-web${NC}"
echo -e "Verify kernel serial driver:"
echo -e "  ${YELLOW}ls -l /dev/ttyXRUSB*${NC}"
echo -e "Access web interface at: ${GREEN}http://localhost:8080${NC}"
echo -e "For group changes to take effect, please log out and back in."
echo -e "${BLUE}==================================================================${NC}"
