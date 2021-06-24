#!/bin/bash

# build all attacks in all variations

cd "$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

. common.sh

main () {
    # build all attacks in all variations
    # settings are passed as macro definitions to the compiler
    # SAMPLE_COUNT is adjusted so that every execution takes on the order of 1 second

    build sidechannel.c sidechannel_reload -DMODE_FR -DVICTIM_STORE -DSAMPLE_COUNT=10000
    build sidechannel.c sidechannel_flush  -DMODE_FF -DVICTIM_STORE -DSAMPLE_COUNT=10000

    build misprediction.c misprediction_cond -DMODE_COND -DCACHE_FR -DSAMPLE_COUNT=10000
    build misprediction.c misprediction_ind  -DMODE_IND  -DCACHE_FR -DSAMPLE_COUNT=10000
    build misprediction.c misprediction_ret  -DMODE_RET  -DCACHE_FR -DSAMPLE_COUNT=10000

    build ridl.c ridl_default   -DTARGET_LFB -DFAULT_UNMAPPED  -DVICTIM_STORE -DCACHE_FR
    build ridl.c ridl_ff        -DTARGET_LFB -DFAULT_UNMAPPED  -DVICTIM_STORE -DCACHE_FF
    build ridl.c ridl_lp        -DTARGET_LP  -DFAULT_UNMAPPED  -DVICTIM_LOAD  -DCACHE_FR
    build ridl.c ridl_load      -DTARGET_LFB -DFAULT_UNMAPPED  -DVICTIM_LOAD  -DCACHE_FR
    build ridl.c ridl_signal    -DTARGET_LFB -DFAULT_SIGNAL    -DVICTIM_STORE -DCACHE_FR
    build ridl.c ridl_transient -DTARGET_LFB -DFAULT_TRANSIENT -DVICTIM_STORE -DCACHE_FR

    build zombieload.c zombieload_default   -DVICTIM_STORE -DFAULT_SIGNAL    -DCACHE_FR
    build zombieload.c zombieload_ff        -DVICTIM_STORE -DFAULT_SIGNAL    -DCACHE_FF
    build zombieload.c zombieload_load      -DVICTIM_LOAD  -DFAULT_SIGNAL    -DCACHE_FR
    build zombieload.c zombieload_transient -DVICTIM_LOAD  -DFAULT_TRANSIENT -DCACHE_FR

    build wtf.c wtf_default   -DTARGET_NONCANON -DVICTIM_STORE -DFAULT_SIGNAL    -DCACHE_FR
    build wtf.c wtf_ff        -DTARGET_NONCANON -DVICTIM_STORE -DFAULT_SIGNAL    -DCACHE_FF
    build wtf.c wtf_transient -DTARGET_NONCANON -DVICTIM_STORE -DFAULT_TRANSIENT -DCACHE_FR
    build wtf.c wtf_kernel    -DTARGET_KERNEL   -DVICTIM_STORE -DFAULT_SIGNAL    -DCACHE_FR

    build storetoleak.c storetoleak_default   -DFAULT_SIGNAL    -DCACHE_FR -DVICTIM_STORE -DSAMPLE_COUNT=10000
    build storetoleak.c storetoleak_ff        -DFAULT_SIGNAL    -DCACHE_FF -DVICTIM_STORE -DSAMPLE_COUNT=10000
    build storetoleak.c storetoleak_transient -DFAULT_TRANSIENT -DCACHE_FR -DVICTIM_STORE -DSAMPLE_COUNT=10000

    build cyclecount.c cyclecount_cpuid  -DMODE_CPUID  -DSAMPLE_COUNT=10000000
    build cyclecount.c cyclecount_fences -DMODE_FENCES -DSAMPLE_COUNT=10000000
}

build () {
    # build c code into binary

    local src="$1"
    local dst="$2"
    local defs=("${@:3}")

    mkdir -p build

    run_log clang -O3 -flto -ggdb3 \
        -Wall -Wextra -Werror \
        -Wconversion -Wsign-conversion \
        -o "build/$dst" "$src" common.c \
        "${defs[@]}"
}

main "$@"
