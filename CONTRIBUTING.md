# Contributing to NvFlux

## Project Layout

```
src/
  main.c      — argument parsing, privilege checks, exit codes
  nvidia.c/h  — all nvidia-smi interaction (touch this for driver quirks)
  profile.c/h — profile name mapping, clock-selection logic
  state.c/h   — XDG state file (~/.local/state/nvflux/state)
include/
  nvflux.h    — public types (Profile enum) and version string
tests/
  test_parse.c — unit tests that run without a GPU or root
```

New functionality belongs in the most specific file above.
If `nvidia-smi` gains a new flag or workaround, put it in `nvidia.c` only.

## Build & Test Requirements

- GCC ≥ 9 or Clang ≥ 10
- CMake ≥ 3.10
- `nvidia-smi` is **not** required to build or run the unit tests

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
ctest --output-on-failure
```

## Writing Tests

Add tests to `tests/test_parse.c`.
Use the `CHECK(expr)` macro — it prints the failed line and exits 1.
Never use `assert()`: it is silenced in Release builds (`-DNDEBUG`).

```c
static void test_my_feature(void) {
    int got = my_function(input);
    CHECK(got == expected);
}
```

Register it at the bottom of `main()` in `test_parse.c`.

Tests must not require root or a real GPU.
Mock `nvidia-smi` output by testing `nv_parse_clocks()` /
`profile_from_str()` / `profile_to_str()` directly.

## Coding Style

- C11, `_POSIX_C_SOURCE=200809L`.
- 4-space indentation, no tabs.
- `snake_case` for functions and variables; `UPPER_CASE` for macros and enum values.
- Every public function has a one-line comment in its `.h` file.
- Keep functions short (aim for < 40 lines). Split helpers if needed.
- No `system()`, no `popen()`, no shell interpolation of user input.

## Submitting Changes

1. Fork the repository and create a feature branch.
2. For driver workarounds include the nvidia-smi version and GPU SKU where you observed the issue.
3. Run `ctest` and confirm 0 failures before opening a pull request.
4. Keep commits focused; one logical change per commit.
5. Add or update documentation in `docs/` if behaviour changes.

## Reporting Bugs

Open a GitHub issue with:
- `nvflux --version`
- `nvidia-smi --version`
- GPU model (`nvidia-smi --query-gpu=name --format=csv,noheader`)
- The exact command that failed and its output
- Distribution and kernel version
