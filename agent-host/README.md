# Bounded Claude Code agent on a tailnet Pi

Run an always-on Claude Code agent on a Raspberry Pi that lives on your tailnet,
drive it from the **Claude mobile app** (Remote Control), and let it manage your
other devices over Tailscale — **wide but not unrestricted**, enforced by tailnet
ACLs rather than by hope.

Execution, filesystem, and network access all stay on **your** Pi. Only model
calls leave the box (to `api.anthropic.com`). The Anthropic cloud never touches
your LAN.

## How the pieces fit

```
  Claude mobile app  --(Remote Control, outbound 443)-->  claude on the Pi
                                                            |
                                            tailnet (tag:agent) + ACLs
                                                            v
                                        tag:devices (your other Pis), SSH/HTTP
```

| File | What it is |
|---|---|
| `setup-agent-pi.sh` | Provisions the Pi: Tailscale, dedicated `claude` user, Claude Code, settings, systemd unit |
| `claude-agent-settings.json` | Bounded tool-permission allow/deny for unattended runs |
| `claude-remote-control.service` | systemd unit running `claude remote-control` |
| `tailscale-acl.hujson` | Tailnet policy: `tag:agent` reaches `tag:devices` only; key-less Tailscale SSH |
| `inventory-tailnet.sh` | The original goal — enumerate your Pis (tailnet membership + LAN sweep) |
| `provision-agent-ssh.sh` | Optional dedicated SSH key, only if you skip Tailscale SSH |

## Hard constraints (verified against the docs)

- **Remote Control needs a Claude subscription login** (Pro/Max/Team/Enterprise).
  **API keys and `setup-token` long-lived tokens do NOT work** for Remote Control —
  they're inference-only. You must do an interactive `/login` once.
- **Pairing doesn't persist across process restarts.** After a reboot or a service
  restart you re-attach from the app's session list (it shows up by `--name`,
  e.g. `myhost-agent`). The credentials login persists; only the live pairing doesn't.
- Confirm exact subcommands/flags on your installed version with
  `claude --help` and `claude remote-control --help` — the base
  `claude remote-control` is stable; cosmetic flags may differ.

---

## Runbook — get connected and let Claude loose

### 1. Set tags in the tailnet ACL (from any browser)
Open <https://login.tailscale.com/admin/acls>, merge in `tailscale-acl.hujson`,
and edit `tag:devices` membership (the Admin console → Machines → each device →
*Edit ACL tags*) so your other Pis are tagged `tag:devices`. Anything you want
off-limits → `tag:infra` (the agent has no rule to reach it).

### 2. Provision the agent Pi (on the Pi, over SSH)
```bash
git clone <this-repo> && cd <repo>/agent-host
sudo ./setup-agent-pi.sh
```

### 3. Join the tailnet as the agent identity (interactive, once)
```bash
sudo -u claude -H tailscale up --ssh --advertise-tags=tag:agent
```
Approve the node in the admin console if tag advertising requires it.

### 4. Log Claude in (interactive, once — this is the subscription login)
```bash
sudo -u claude -H /home/claude/.local/bin/claude
```
Inside the session run `/login`. On a headless box press `c` to copy the URL,
open it in a browser on your phone/laptop, finish the OAuth, and paste the code
back if prompted. Credentials land in `/home/claude/.claude/.credentials.json`
(mode 600) and persist. Type `/exit`.

### 5. Start the always-on service
```bash
sudo systemctl enable --now claude-remote-control
journalctl -u claude-remote-control -f    # watch it come up
```

### 6. Attach from the Claude app
Open the Claude mobile app → Code, find the session named `<host>-agent`
(or scan the QR / open the session URL from the journal logs). You're now typing
into the Pi from your phone — same prompt box, on-network execution.

### 7. Let it loose — first job: identify your Pis
From the app, tell the agent:
> Run `./inventory-tailnet.sh` and give me a labeled roster of my Pis — which are
> online, their tailnet IPs, OS, tags, and which LAN device looks like which project.

That parses `tailscale status` (every tailnet device, cross-network, no scanning)
and sweeps the local LAN for Raspberry Pi MAC OUIs. Then point it at any
`tag:devices` host over Tailscale SSH (`ssh agent@<host>`) to inventory, patch, or
improve — bounded by your ACLs.

---

## Tightening / loosening the leash

- **Broader autonomy:** add patterns to `allow` in `claude-agent-settings.json`
  (e.g. `Bash(apt *)`), or raise `defaultMode`. Avoid `bypassPermissions` outside a
  sandbox/VM.
- **Narrower reach:** trim ports in the ACL, move sensitive boxes to `tag:infra`,
  or switch Tailscale SSH rules from `accept` to `check` (adds re-auth, but breaks
  fully-unattended runs).
- **Audit:** `journalctl -u claude-remote-control`, Tailscale SSH session logging,
  and the agent's own transcript in the app.
