#!/usr/bin/env bash
# Enumerate devices the agent can see -- the original goal: "identify my Pis."
# Two sources:
#   1. tailnet membership (cross-network, no scanning needed) via tailscale status
#   2. the local LAN this box sits on, via an ARP/ping sweep + Pi OUI matching
# Read-only. Safe to hand to the agent. Requires: tailscale, nmap (optional), jq.
set -euo pipefail

echo "============================================================"
echo " TAILNET DEVICES (hostname / OS / online / tailnet IP / tags)"
echo "============================================================"
if command -v tailscale >/dev/null 2>&1; then
  if command -v jq >/dev/null 2>&1; then
    tailscale status --json | jq -r '
      [ .Self ] + ( .Peer // {} | to_entries | map(.value) ) | .[] |
      [ (.HostName // "?"),
        (.OS // "?"),
        (if .Online then "online" else "offline" end),
        ((.TailscaleIPs // ["-"])[0]),
        ((.Tags // ["-"]) | join(",")) ] | @tsv' \
    | column -t -s $'\t'
  else
    echo "(install jq for parsed output) -- raw:"; tailscale status
  fi
else
  echo "tailscale not installed on this box."
fi

echo
echo "============================================================"
echo " LOCAL LAN SWEEP (this box's physical network)"
echo "============================================================"
SUBNET="${1:-}"
if [[ -z "$SUBNET" ]]; then
  # Best-effort: derive the primary IPv4 /24 from the default route iface.
  IFACE="$(ip route show default 2>/dev/null | awk '/default/ {print $5; exit}')"
  CIDR="$(ip -o -4 addr show "${IFACE}" 2>/dev/null | awk '{print $4; exit}')"
  if [[ -n "${CIDR:-}" ]]; then
    SUBNET="$(echo "$CIDR" | sed -E 's#\.[0-9]+/[0-9]+#.0/24#')"
  fi
fi

if [[ -n "${SUBNET:-}" ]] && command -v nmap >/dev/null 2>&1; then
  echo "Sweeping ${SUBNET} (pass a CIDR as \$1 to override)..."
  # -sn = ping/host-discovery only, no port scan.
  sudo nmap -sn "${SUBNET}" >/dev/null 2>&1 || true
fi

echo
echo "Likely Raspberry Pis on this LAN (ARP table, Pi OUI prefixes):"
arp -an 2>/dev/null \
  | grep -iE 'b8:27:eb|dc:a6:32|e4:5f:01|d8:3a:dd|28:cd:c1|2c:cf:67' \
  || echo "  (none matched -- Pi may be powered off, on another VLAN, or using a USB/3rd-party NIC)"
