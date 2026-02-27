# fish completion for nvflux
# Install to:
#   /usr/share/fish/vendor_completions.d/nvflux.fish   (system-wide)
#   ~/.config/fish/completions/nvflux.fish             (user)

# Suppress default file completion for nvflux
complete -c nvflux -f

# Helper: true when no subcommand has been given yet
set -l __nvflux_cmds ultra performance balanced powersave auto status clock

# ── Subcommands ────────────────────────────────────────────────────────────────
complete -c nvflux \
    -n "not __fish_seen_subcommand_from $__nvflux_cmds" \
    -a ultra \
    -d 'Lock GPU core + memory to max clocks (PowerMizer: Prefer Max Performance)'

complete -c nvflux \
    -n "not __fish_seen_subcommand_from $__nvflux_cmds" \
    -a performance \
    -d 'Lock memory clock to the highest supported tier'

complete -c nvflux \
    -n "not __fish_seen_subcommand_from $__nvflux_cmds" \
    -a balanced \
    -d 'Lock memory clock to the mid-range tier'

complete -c nvflux \
    -n "not __fish_seen_subcommand_from $__nvflux_cmds" \
    -a powersave \
    -d 'Lock memory clock to the lowest supported tier'

complete -c nvflux \
    -n "not __fish_seen_subcommand_from $__nvflux_cmds" \
    -a auto \
    -d 'Unlock all clocks (driver-managed, PowerMizer: Adaptive)'

complete -c nvflux \
    -n "not __fish_seen_subcommand_from $__nvflux_cmds" \
    -a status \
    -d 'Show last saved profile (no root needed)'

complete -c nvflux \
    -n "not __fish_seen_subcommand_from $__nvflux_cmds" \
    -a clock \
    -d 'Print current memory clock in MHz'

# ── Flags ──────────────────────────────────────────────────────────────────────
complete -c nvflux \
    -n "not __fish_seen_subcommand_from $__nvflux_cmds" \
    -l restore \
    -d 'Re-apply the last saved profile'

complete -c nvflux -l version -s v -d 'Print version and exit'
complete -c nvflux -l help    -s h -d 'Print usage information and exit'
