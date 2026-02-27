# Security Model

## Principles

- **Allowlist**: only profile names from the hardcoded `profiles[]` table are accepted; `--restore` validates the saved name against this table before acting.
- **No shell**: all subprocess calls use `fork`/`execv` directly — no `system()`, no string interpolation into commands.
- **Privilege scope**: root (`geteuid()==0`) is used only for `nvidia-smi` calls. The `status` command runs entirely unprivileged.
- **State file ownership**: written as real UID (not root), mode 0600, in `~/.local/state/nvflux/state`.

## Installation Note

Installing a setuid-root binary has security implications. Review `src/` before deploying. To restrict to a group:
```sh
sudo chown root:wheel /usr/local/bin/nvflux
sudo chmod 4750       /usr/local/bin/nvflux
```

## Known Limitations

- `ultra` locks both GPU core and memory clocks; `auto` unlocks both.
  `performance`, `balanced`, and `powersave` lock only the memory clock and
  explicitly reset the GPU core clock to driver-managed (Adaptive).
- Memory clock locking (`--lock-memory-clocks`) is not supported on NVIDIA
  Hopper architecture GPUs (H100 etc.). NvFlux automatically falls back to
  `--lock-memory-clocks-deferred`, which requires a kernel-module reload to
  take effect (`sudo rmmod nvidia && sudo modprobe nvidia`).
- Applies to all GPUs (nvidia-smi default). Multi-GPU targeting via `-i` is a possible future addition.
- No rate limiting on calls from the same user.
