// determine average overhead of latency measurement ordered by cpuid or by fences

#include "common.h"

#include <string.h>

#if defined(MODE_CPUID)

// execute a single cpuid instruction
ALWAYS_INLINE static void cpuid(void) {
    // inputs: eax=0
    // clobbers: ebx, ecx, edx
    asm("cpuid" ::"a"(0) : "ebx", "ecx", "edx");
}

// measure overhead of cyclecount ordered by cpuid
NOINLINE static cyclecount_t cyclecount_nothing(void) {
    cpuid();
    cyclecount_t start = __builtin_ia32_rdtsc();
    cpuid();
    // here would be the memory access
    cpuid();
    cyclecount_t end = __builtin_ia32_rdtsc();
    cpuid();
    return end - start;
}

#elif defined(MODE_FENCES)

// measure overhead of cyclecount ordered by fences
NOINLINE static cyclecount_t cyclecount_nothing(void) {
    uint32_t unused;
    __builtin_ia32_mfence();
    cyclecount_t start = __builtin_ia32_rdtscp(&unused);
    __builtin_ia32_lfence();
    // here would be the memory access
    cyclecount_t end = __builtin_ia32_rdtscp(&unused);
    __builtin_ia32_lfence();
    return end - start;
}

#else
#error "Invalid mode"
#endif

void attacker(void) {
    // determine average overhead of cyclecount across SAMPLE_COUNT samples

    cyclecount_t total = 0;

    for (size_t i = 0; i < SAMPLE_COUNT; i++)
        total += cyclecount_nothing();

    printf("%lg\n", (double)total / SAMPLE_COUNT);
}
