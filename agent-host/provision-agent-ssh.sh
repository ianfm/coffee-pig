#!/usr/bin/env bash
# OPTIONAL: only needed if you are NOT using Tailscale SSH (tailscale-acl.hujson
# governs SSH centrally, no keys to distribute -- prefer that). Use this when a
# target device isn't on the tailnet or doesn't have Tailscale SSH enabled.
#
# Creates a DEDICATED, agent-only SSH key (never reuse your personal key) and
# prints the one line to authorize it on a target as a low-priv 'agent' user.
#
# Run as the agent user on the agent Pi:  ./provision-agent-ssh.sh
set -euo pipefail

KEY="${HOME}/.ssh/agent_ed25519"
install -d -m 0700 "${HOME}/.ssh"

if [[ ! -f "${KEY}" ]]; then
  ssh-keygen -t ed25519 -N "" -C "claude-agent@$(hostname)" -f "${KEY}"
  echo "Created ${KEY}"
else
  echo "Reusing existing ${KEY}"
fi

echo
echo "On each TARGET device, create a low-priv user and authorize this key:"
echo "----------------------------------------------------------------------"
echo "sudo adduser --disabled-password --gecos '' agent"
echo "sudo install -d -m700 -o agent -g agent /home/agent/.ssh"
echo "echo '$(cat "${KEY}.pub")' | sudo tee -a /home/agent/.ssh/authorized_keys"
echo "sudo chown agent:agent /home/agent/.ssh/authorized_keys && sudo chmod 600 /home/agent/.ssh/authorized_keys"
echo "----------------------------------------------------------------------"
echo "Then the agent connects with:  ssh -i ${KEY} agent@<target>"
echo
echo "Scope it down further on the target by prefixing authorized_keys with, e.g.:"
echo '  from="100.64.0.0/10",no-agent-forwarding,no-X11-forwarding ssh-ed25519 AAAA...'
