# NvFlux

A minimal, secure setuid-root helper for unprivileged NVIDIA GPU profile management.

[![License: MIT](https://img.shields.io/badge/license-MIT-red.svg)](LICENSE)

## Overview

nvflux allows desktop users to switch NVIDIA GPU power profiles without `sudo` by providing a carefully constrained interface to `nvidia-smi` operations. It's designed for:

- **Desktop Linux users** who want convenient GPU profile switching
- **Laptop users** needing battery vs performance trade-offs
- **Multi-boot systems** where persistence settings need per-session control

**Key Features:**
- ✅ No security risk (allowlist-only commands)
- ✅ No shell invocation (direct exec calls)
- ✅ Per-user state persistence
- ✅ Distro-agnostic (no systemd/compositor dependencies)
- ✅ Well-tested with unit tests

## Quick Start

```bash
# 1. Check dependencies
./scripts/check-deps.sh

# 2. Install (requires root)
sudo ./scripts/install.sh

# 3. Use it
nvflux performance   # Max performance
nvflux balanced      # Balanced mode
nvflux powersaver    # Power saving
nvflux reset         # Auto (driver managed)
nvflux status        # Show current profile
```

## Documentation

- **[Installation Guide](docs/INSTALLATION.md)** - Detailed setup per distro
- **[Security Model](docs/SECURITY.md)** - How nvflux stays safe
- **[Contributing](CONTRIBUTING.md)** - Development guidelines

## Usage Examples

### Basic Commands

```bash
# Switch profiles
nvflux performance
nvflux balanced
nvflux powersaver

# Reset to automatic
nvflux auto         # or: nvflux reset

# Query status
nvflux status       # Show saved profile
nvflux clock        # Show current memory clock (MHz)

# Restore on boot
nvflux --restore    # Apply saved profile

# Help
nvflux --help
nvflux --version
man nvflux
```

### Integration Examples

See [examples/](examples/) for:
- [profile-switcher.sh](examples/profile-switcher.sh) - Interactive menu
- [rofi-launcher.sh](examples/rofi-launcher.sh) - GUI launcher for i3/Sway

### Autostart Setup

**i3/Sway** (`~/.config/i3/config`):
```
exec --no-startup-id nvflux --restore
```

**systemd user service** - see [docs/INSTALLATION.md](docs/INSTALLATION.md#autostart-configuration)

## Architecture

```
nvflux (setuid root)
├── src/main.c          # Entry point, version handling
├── src/nvflux.c        # Core logic (privilege management, nvidia-smi calls)
├── include/nvflux.h    # Public API
└── tests/test_nvflux.c # Unit tests (no root required)
```

**Security Model:**
- Validates all commands against allowlist
- No arbitrary string injection into nvidia-smi
- State files owned by real UID, not root
- See [docs/SECURITY.md](docs/SECURITY.md) for full analysis

## Building

### With CMake (Recommended)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest -V  # Run tests
```

### Manual Install

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install manually
sudo install -Dm4755 build/nvflux /usr/local/bin/nvflux
sudo gzip -c man/nvflux.1 > /usr/local/share/man/man1/nvflux.1.gz
```

## Requirements

- **Runtime:** NVIDIA drivers with `nvidia-smi`
- **Build:** C compiler (GCC/Clang), CMake 3.10+, gzip

See [docs/INSTALLATION.md](docs/INSTALLATION.md) for distro-specific package names.

## Testing

```bash
cd build
ctest -V          # Run all tests
./test_nvflux     # Run test binary directly
```

Tests cover:
- Clock parsing logic
- Input validation
- Edge cases (empty input, sorting)

## Troubleshooting

**"nvidia-smi not found"**
- Install NVIDIA drivers for your distro
- Ensure `nvidia-smi` is in PATH or `/usr/bin`

**"No devices were found"**
- NVIDIA kernel module not loaded: `sudo modprobe nvidia`
- Wrong driver version for your GPU

**"Permission denied" after install**
- Check setuid bit: `ls -l /usr/local/bin/nvflux` (should show `-rwsr-xr-x`)
- Re-run: `sudo chown root:root /usr/local/bin/nvflux && sudo chmod 4755 /usr/local/bin/nvflux`

## Uninstallation

```bash
sudo ./scripts/uninstall.sh
```

Removes binary, man page, and user state directory.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Contributions welcome!

**Before submitting:**
1. Add unit tests for logic changes
2. Run `ctest` to verify
3. Document security implications if touching privilege code

## License

See [LICENSE](LICENSE) file.

## See Also

- [nvidia-smi documentation](https://developer.nvidia.com/nvidia-system-management-interface)
- [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html)
