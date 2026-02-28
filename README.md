# NvFlux

Fix HDMI/DisplayPort audio dropouts on NVIDIA GPUs — and take control of GPU clock profiles from the command line.

[![License: MIT](https://img.shields.io/badge/license-MIT-red.svg)](LICENSE)

## The Problem

If your monitor is connected to your NVIDIA GPU via HDMI or DisplayPort and you use its
built-in speakers or audio output, you have probably heard this: random audio stutters,
brief dropouts, or sudden silence — most often during quiet desktop moments like
scrolling, switching windows, or leaving the machine idle.

This is not a driver bug — it is the GPU power management working as designed.
NVIDIA continuously shifts the GPU between P-states (performance states) based on
instantaneous load. The GPU's memory controller and its HDMI/DP audio controller share
the same clock domain. Every time the driver raises or lowers the memory clock to match
the current P-state, the audio controller's clock source is momentarily disrupted, and
the audio stream drops out.

**The fix is simple: lock the memory clock.** Pinning it to any fixed frequency stops
P-state transitions on the memory bus. The audio controller gets a stable clock and the
dropouts stop completely.

```bash
# Install once
sudo ./scripts/install.sh

# Fix the audio — one command, no reboot needed
nvflux powersave

# Persist it across reboots (add to your WM autostart or systemd user service)
nvflux --restore
```

`powersave` locks to the lowest memory tier — enough to prevent P-state transitions,
with minimal extra power draw or fan noise. Use `balanced` or `performance` if you also
need higher memory bandwidth for rendering or gaming.

## Installation

```bash
./scripts/check-deps.sh   # verify nvidia-smi, gcc, cmake are present
sudo ./scripts/install.sh # build and install setuid-root binary
```

See [docs/INSTALLATION.md](docs/INSTALLATION.md) for per-distro dependency commands and
autostart setup (i3/Sway exec line, systemd user service).

## Profiles

| Command       | Memory clock | GPU core | Use when                                   |
|---------------|--------------|----------|--------------------------------------------|
| `powersave`   | min tier     | adaptive | Audio fix, idle desktop, battery saving    |
| `balanced`    | mid tier     | adaptive | General desktop + light rendering          |
| `performance` | max tier     | adaptive | Gaming, GPU compute, heavy rendering       |
| `ultra`       | max tier     | max      | Benchmarking, maximum sustained throughput |
| `auto`        | driver       | driver   | Revert to default dynamic P-state control  |

## All Commands

```bash
nvflux powersave     # Lock memory to lowest tier  (recommended for audio fix)
nvflux balanced      # Lock memory to mid tier
nvflux performance   # Lock memory to highest tier
nvflux ultra         # Lock memory + GPU core to maximum
nvflux auto          # Unlock everything — back to driver-managed P-states

nvflux status        # Show the last saved profile
nvflux clock         # Print the current memory clock in MHz
nvflux --restore     # Re-apply the last saved profile (for autostart)
nvflux --version
nvflux --help
```

## Autostart

Re-apply your profile automatically on login so the fix survives reboots.

**i3 / Sway** (`~/.config/i3/config` or `~/.config/sway/config`):
```
exec --no-startup-id nvflux --restore
```

**systemd user service** — see [docs/INSTALLATION.md](docs/INSTALLATION.md#autostart-configuration).

## Why a Setuid Binary?

`nvidia-smi` clock-locking commands require root. Rather than forcing `sudo` on every
invocation — which breaks seamless autostart and scripting — nvflux is installed setuid
root (`-rwsr-xr-x`). It drops back to the real user UID immediately after the privileged
operation. Read-only commands (`status`, `clock`) never use the elevated privilege at all.

The attack surface is deliberately minimal: all arguments are validated against a fixed
allowlist, `nvidia-smi` is exec'd directly with no shell, and the state file is always
owned by the real user. See [docs/SECURITY.md](docs/SECURITY.md) for the full model.

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest -V          # run unit tests (no GPU or root required)
```

**gcc fallback** (no cmake):
```bash
gcc -O2 -std=c11 -Iinclude \
    src/main.c src/nvidia.c src/profile.c src/state.c \
    -o nvflux
```

## Requirements

- **Runtime:** NVIDIA drivers with `nvidia-smi` (Volta+ for clock locking)
- **Build:** C11 compiler, CMake 3.10+, gzip

| Distro        | Dependencies                                                        |
|---------------|---------------------------------------------------------------------|
| Arch Linux    | `sudo pacman -S nvidia-utils base-devel cmake gzip`                 |
| Debian/Ubuntu | `sudo apt install build-essential cmake gzip`          |
| Fedora        | `sudo dnf install @development-tools cmake gzip`                    |
| openSUSE      | `sudo zypper install -t pattern devel_C_C++ cmake gzip`             |
| Solus         | `sudo eopkg it -c system.devel`  |
| Void Linux    | `sudo xbps-install -S base-devel cmake gzip nvidia-utils`           |

## Troubleshooting

**Audio dropout still happens after `nvflux powersave`**  
Confirm the clock is actually locked: `nvflux clock` — run it a few times and check the
value is stable. If it keeps changing, the lock did not apply (see Hopper note below).

**Memory clock not changing (Hopper / Ada Lovelace GPU)**  
nvflux automatically falls back to `--lock-memory-clocks-deferred`. The lock takes
effect after a driver reload:
```bash
sudo rmmod nvidia_uvm nvidia_drm nvidia_modeset nvidia && sudo modprobe nvidia
```

**"nvidia-smi not found"**  
Install NVIDIA drivers; ensure `nvidia-smi` is in PATH or `/usr/bin`.

**"No devices were found"**  
Kernel module not loaded: `sudo modprobe nvidia`

**"Permission denied" after install**  
`ls -l /usr/local/bin/nvflux` should show `-rwsr-xr-x`.  
Fix: `sudo chown root:root /usr/local/bin/nvflux && sudo chmod 4755 /usr/local/bin/nvflux`

## Architecture

```
include/nvflux.h    — Profile enum, version string
src/
  main.c            — argument parsing, privilege checks, exit codes
  nvidia.h/.c       — all nvidia-smi interaction (find, exec, parse, lock)
  profile.h/.c      — profile apply logic, name ↔ enum mapping
  state.h/.c        — XDG state file (~/.local/state/nvflux/state)
tests/
  test_parse.c      — unit tests (no GPU, no root needed)
```

## Uninstallation

```bash
sudo ./scripts/uninstall.sh
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

See [LICENSE](LICENSE).

## See Also

- [nvidia-smi documentation](https://docs.nvidia.com/deploy/nvidia-smi/index.html)
- [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html)

