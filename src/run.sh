#!/bin/bash

# execute build.sh, run attacks and collect data, execute eval_and_plot.py

cd "$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

. common.sh

# number of times each attack is executed, evaluation was done with 1000
RUN_COUNT=10
# seconds to sleep between each run
RUN_DELAY=0.1

main () {
    # check that no attacks are already running
    error_if_running
    # check system configuration for errors
    check_system_config

    mkdir -p build data

    heading "Building"

    ./build.sh

    heading "Collecting measurements"

    # run attacks and collect data
    # second argument is list of fields printed by the attack
    collect sidechannel   cached,uncached
    collect misprediction rate_hits,cycles_train,cycles_attack
    collect ridl          rate_hits,cycles_leak,cycles_decode,total_time
    collect wtf           rate_hits,cycles_leak,cycles_decode,total_time
    # zombieload has to be run as root, execute it with sudo
    run_wrapper=(sudo)
    collect zombieload    rate_hits,cycles_leak,cycles_decode,total_time
    run_wrapper=()
    collect storetoleak   rate_hits_mapped,rate_hits_unmapped,time_mapped,time_unmapped

    heading "Calculating metrics"

    ./eval_and_plot.py
}

collect () {
    # execute all binaries with given prefix and collect produced data

    local prefix="$1"
    local fields="$2"

    echo "$prefix:" >&2
    echo >&2

    # iterate over all binaries with the given prefix
    local bin
    for bin in ./build/"$prefix"_*; do
        # extract last path component
        local name="${bin##*/}"
        # execute and collect data
        collect_single "$name" "$fields"
    done

    echo >&2
}

collect_single () {
    # execute binary and collect produced data into csv file

    local name="$1"
    local fields="$2"

    {
        echo "$fields"
        run_progress "$RUN_COUNT" "./build/$name"
    } > "data/$name.csv"
}

main "$@"
