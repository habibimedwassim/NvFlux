# fish completion for nvflux
set -l cmds ultra performance balanced powersave auto status clock --restore --help --version

complete -c nvflux -f -n "not __fish_seen_subcommand_from $cmds"
complete -c nvflux -f -a ultra       -d "Lock memory + GPU core to max (PowerMizer: Prefer Max Performance)"
complete -c nvflux -f -a performance -d "Lock memory to highest tier"
complete -c nvflux -f -a balanced    -d "Lock memory to mid-range tier"
complete -c nvflux -f -a powersave   -d "Lock memory to lowest tier"
complete -c nvflux -f -a auto        -d "Unlock all clocks (driver-managed)"
complete -c nvflux -f -a status      -d "Show last saved profile"
complete -c nvflux -f -a clock       -d "Print current memory clock in MHz"
complete -c nvflux -f -a --restore   -d "Re-apply last saved profile"
complete -c nvflux -f -a --help      -d "Show help"
complete -c nvflux -f -a --version   -d "Print version"
