#!/bin/bash
# Sets User= in both service files to the invoking user.
# Handles being called via sudo (uses SUDO_USER if available).

TARGET_USER="${SUDO_USER:-$(whoami)}"
sed -i "s/^#User=$/User=$TARGET_USER/" coffee-roaster.service web/coffee-roaster-web.service panel/coffee-roaster-panel.service
echo "Set User=$TARGET_USER in service files."
