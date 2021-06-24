# utilities used by run.sh and build.sh

set -u

run_wrapper=()

heading () {
    # print heading surrounded by empty lines

    echo >&2
    echo "---------------- $1 ----------------" >&2
    echo >&2
}

run_log () {
    # print command and execute it, exit on error

    echo "$*" >&2
    "$@" || exit
}

run_progress () {
    # run command multiple times, with progress bar

    local count="$1"
    local prog=("${@:2}")

    echo "Running ${prog[*]}" >&2

    local i
    for i in $(seq "$count"); do

        # execute command
        "${run_wrapper[@]}" "${prog[@]}"
        ret="$?"

        # check for failure
        if [[ "$ret" -ne 0 ]]; then
            echo "${prog[*]} failed" >&2
            exit "$ret"
        fi

        # print progress bar
        progress "$i" "$count"

        # sleep a bit to separate measurements in time
        sleep "${RUN_DELAY:-0}"
    done
}

progress () {
    # print progress bar

    local cur="$1"
    local max="$2"
    if [[ "$cur" -gt "$max" ]]; then
        cur="$max"
    fi

    local width=50
    local icons=(" " "▏" "▎" "▍" "▌" "▋" "▊" "▉")
    local icon_full="█"
    local i

    # number of possible progress states
    local states=$(( $width * ${#icons[@]} ))
    # current progress state
    local state=$(( $cur * $states / $max ))
    # number of completely filled icons
    local filled=$(( $state / ${#icons[@]} ))
    # current icon index
    local icon_idx=$(( $state % ${#icons[@]} ))

    printf "[" >&2
    # print completely filled icons
    for i in $(seq "$filled"); do
        printf "$icon_full" >&2
    done
    # print current icon
    if [[ "$filled" -lt "$width" ]]; then
        printf "${icons[$icon_idx]}" >&2
    fi
    # print completely free icons
    for i in $(seq "$(( $width - $filled - 1 ))"); do
        printf " " >&2
    done
    printf "]" >&2
    # print progress counter
    printf "  %d / %d" "$cur" "$max" >&2
    printf "        " >&2
    # print carriage return if not done, newline if done
    if [[ "$cur" -lt "$max" ]]; then
        printf "\r" >&2
    else
        printf "\n" >&2
    fi
}

error_if_running () {
    # print error and exit if any binaries from build/ are running

    local bin
    for bin in ./build/*; do
        # get basename of binary
        bin="${bin##*/}"
        # check if already running
        if pidof -q "$bin"; then
            echo "Error: $bin is already running, terminate it first" >&2
            exit 1
        fi
    done
}

check_system_config () {
    # display warning when system configuration may impact results

    # check cpu frequency governor
    local governor="$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
    if [[ "$governor" != "performance" ]]; then
        echo "CPU Frequency Governor is set to '$governor', consider changing it to 'performance'" >&2
    fi
}
