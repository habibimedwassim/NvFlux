# Security Policy

## Supported Versions

| Version | Supported |
| ------- | --------- |
| 2.x     | Yes       |
| < 2.0   | No        |

## Security Model

### Privilege Handling

nvflux requires root to call `nvidia-smi` with clock-locking flags.
The binary is installed **setuid root** (`-rwsr-xr-x`).

Privilege is dropped as early as possible:
- Commands that do not need root (`status`, `clock`) run entirely as the real user.
- Only `profile_apply()` and `--restore` retain elevated privileges.
- The setuid approach was chosen over `sudo` to allow desktop autostart without a terminal.

### Command Validation

All `nvidia-smi` invocations use **`execv()` with an explicit argument list**.
There is no shell interpolation (`system()` is never called).
Untrusted string input (profile names) is validated against a fixed allowlist
via `profile_from_str()` before any value reaches `nvidia-smi`.

### State File

The state file at `~/.local/state/nvflux/state` stores only the last active
profile name (a short ASCII string).

- Created with mode `0600`.
- Ownership is `fchown`-ed to the **real UID** immediately after creation,
  preventing root from owning the file in the user's home directory.
- The file is read by `--restore` and validated through `profile_from_str()`
  before use; a corrupted/malicious state file will produce an "unknown profile"
  error and exit cleanly.

### Attack Surface Reduction

| Vector | Mitigation |
| ------ | ---------- |
| Argument injection | Fixed argument arrays; no shell expansion |
| Path traversal | nvidia-smi path resolved once at startup via `nv_init()`, not user-controlled |
| Buffer overflows | Stack buffers sized with `PATH_MAX`/`NV_MAX_CLOCKS` constants; `fgets`/`snprintf` throughout |
| Integer overflows | Clock values are `int` parsed with `strtol`; unused high-range values are discarded |
| Symlink attacks | State directory created with `mkdir -p`; ownership validated after open |

### What nvflux Does NOT Do

- Does **not** expose a network socket or D-Bus interface.
- Does **not** write to system paths other than via the install script.
- Does **not** parse GPU vendor strings or firmware blobs.
- Does **not** store passwords, tokens, or any secret material.

## Reporting a Vulnerability

Please **do not** open a public issue for security vulnerabilities.

Open a [GitHub Security Advisory](https://docs.github.com/en/code-security/security-advisories)
in the repository, or email the maintainer directly (see `git log` for contact).

Include:
- Description of the vulnerability
- Steps to reproduce
- Affected version(s)
- Suggested fix (optional)

You can expect an acknowledgement within **48 hours** and a patch within **7 days**
for confirmed vulnerabilities.
