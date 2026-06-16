#!/usr/bin/env bash
# Provision a Raspberry Pi as a bounded, always-on Claude Code agent.
# Run ON the Pi. Re-runnable (idempotent-ish). Does NOT do the one-time
# interactive `claude` login or the `tailscale up` auth -- those are
# interactive and live in the README runbook.
#
# Usage:  sudo ./setup-agent-pi.sh
set -euo pipefail

AGENT_USER="claude"
AGENT_HOME="/home/${AGENT_USER}"
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ $EUID -ne 0 ]]; then echo "Run with sudo." >&2; exit 1; fi

echo "==> 1/6  Base packages (tailscale, nmap, git, curl)"
if ! command -v tailscale >/dev/null 2>&1; then
  curl -fsSL https://tailscale.com/install.sh | sh
fi
apt-get update -qq
apt-get install -y --no-install-recommends nmap iproute2 net-tools git curl ca-certificates

echo "==> 2/6  Dedicated unprivileged user '${AGENT_USER}'"
if ! id "${AGENT_USER}" >/dev/null 2>&1; then
  useradd -m -s /bin/bash "${AGENT_USER}"
fi
install -d -o "${AGENT_USER}" -g "${AGENT_USER}" "${AGENT_HOME}/workspace" "${AGENT_HOME}/.claude"

echo "==> 3/6  Install Claude Code as '${AGENT_USER}' (native installer)"
sudo -u "${AGENT_USER}" -H bash -lc 'command -v claude >/dev/null 2>&1 || curl -fsSL https://claude.ai/install.sh | bash'

echo "==> 4/6  Bounded permissions -> ${AGENT_HOME}/.claude/settings.json"
install -o "${AGENT_USER}" -g "${AGENT_USER}" -m 0644 \
  "${REPO_DIR}/claude-agent-settings.json" "${AGENT_HOME}/.claude/settings.json"

echo "==> 5/6  systemd unit for Remote Control"
install -m 0644 "${REPO_DIR}/claude-remote-control.service" /etc/systemd/system/
systemctl daemon-reload

echo "==> 6/6  Done with the non-interactive parts."
cat <<EOF

Next (interactive, see README runbook), as the agent user:
  sudo -u ${AGENT_USER} -H tailscale up --ssh --advertise-tags=tag:agent
  sudo -u ${AGENT_USER} -H ${AGENT_HOME}/.local/bin/claude   # then run /login once
Then enable the service:
  sudo systemctl enable --now claude-remote-control
  journalctl -u claude-remote-control -f
EOF
