# Security Considerations

## Setuid Root Design

nvflux is designed as a setuid-root binary with the following security principles:

### Attack Surface Minimization
- **Allowlist approach**: Only 7 commands are permitted (see `allowed_cmds[]` in [src/nvflux.c](../src/nvflux.c))
- **No arbitrary command execution**: All nvidia-smi invocations use hardcoded argument patterns
- **Input validation**: Command names are validated before any privileged operations
- **Path safety**: nvidia-smi path is validated with `access(X_OK)` before use
- **Clock values are queried from the GPU**: No user-supplied clock values are passed to nvidia-smi

### Privilege Management
- Real UID is preserved for state file ownership
- State files are written to user's `~/.local/state/nvflux/` with correct ownership
- Effective UID (root) is only used for nvidia-smi execution

### Safe Execution
- `exec_capture()` uses fork/exec pattern (no shell involvement)
- All buffers have fixed sizes with bounds checking
- No dynamic memory allocation that could leak
- File descriptors are properly closed in all paths

### State File Safety
- State files are per-user in standard XDG location
- Files are created with 0600 permissions (user read/write only)
- `fchown()` ensures correct ownership even when running as root

## Installation Security

**Warning**: Installing setuid-root binaries has security implications. Only install on systems you control and trust.

### Best Practices
1. Review the source code before installation
2. Verify the installer script doesn't modify other system files
3. Restrict `/usr/local/bin/nvflux` permissions if needed (chmod 4750 with restricted group)
4. Monitor system logs for unexpected nvflux usage

### For Distributions
- Consider using polkit instead of setuid for package distribution
- Add AppArmor/SELinux profiles if available
- Package tests and run during build verification

## Known Limitations
- Multi-GPU systems: currently applies settings to all GPUs
- No rate limiting: users can call nvflux repeatedly
- State persistence: relies on filesystem, no locking mechanism