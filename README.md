# NvFlux

A minimal, setuid-root helper for NVIDIA GPU clock profile management on Linux.

[![License: MIT](https://img.shields.io/badge/license-MIT-red.svg)](LICENSE)

## Overview

`nvflux` lets desktop users switch NVIDIA GPU profiles without `sudo` via `nvidia-smi`.
Clock tiers are queried live from the driver — nothing is hard-coded, so it works on any NVIDIA GPU.
All profiles work on **Wayland** and on **driver versions ≤ 580** where the nvidia-settings
PowerMizer dropdown is broken.

| Profile     | Memory clock                     | GPU core clock          | PowerMizer equivalent           |
|-------------|----------------------------------|-------------------------|-----------------------------------|
| ultra       | Locked to max                    | Locked to max           | Prefer Maximum Performance        |
| performance | Locked to highest tier           | Not touched             | —                                 |
| balanced    | Locked to mid-range tier         | Not touched             | —                                 |
| powersave   | Locked to lowest tier            | Not touched             | —                                 |
| auto        | Unlocked (driver managed)        | Unlocked (driver managed) | Adaptive                        |

## Quick Start

```sh
./scripts/check-deps.sh     # check build dependencies
sudo ./scripts/install.sh   # build, install, set setuid bit

nvflux ultra                 # lock GPU + memory to max (gaming/compute)
nvflux performance           # lock memory to max tier only
nvflux balanced              # lock memory to mid tier
nvflux powersave             # lock memory to lowest tier
nvflux auto                  # unlock all clocks
nvflux --restore             # re-apply saved profile
```

## Requirements

| Dependency   | Purpose                         |
|--------------|---------------------------------|
| `nvidia-smi` | Clock locking (required)        |
| GCC / Clang  | Build (required)                |
| CMake ≥ 3.10 | Build (preferred; gcc fallback) |

## Autostart

### i3 / Sway
```
exec --no-startup-id nvflux --restore
```

### GNOME / KDE / XFCE
```ini
# ~/.config/autostart/nvflux-restore.desktop
[Desktop Entry]
Type=Application
Name=nvflux restore
Exec=nvflux --restore
```

### systemd user service
```ini
# ~/.config/systemd/user/nvflux-restore.service
[Unit]
Description=Restore NVIDIA GPU profile
After=graphical-session.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/nvflux --restore

[Install]
WantedBy=default.target
```
```sh
systemctl --user enable --now nvflux-restore.service
```

## Building

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest -V
```

## Security Model

- Input validated against a hardcoded profile table before any action.
- All subprocess calls use `execv` directly — no shell involvement.
- State file owned by real UID (not root), mode 0600.
- See [docs/SECURITY.md](docs/SECURITY.md).

## Uninstall

```sh
sudo ./scripts/uninstall.sh
```

## License

MIT — see [LICENSE](LICENSE).
