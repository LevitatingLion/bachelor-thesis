// evaluate flush+reload or flush+flush as cache-based side-channel attacks

#include "common.h"

#include <string.h>

// mode settings determines if we access and flush or only flush
#if defined(MODE_FF)
#define cyclecount_action cyclecount_flush
#elif defined(MODE_FR)
#define cyclecount_action cyclecount_access_flush
#else
#error "Invalid mode"
#endif

// latencies for cached and uncached cachelines are collected separately
static cyclecount_t samples_cached[SAMPLE_COUNT];
static cyclecount_t samples_uncached[SAMPLE_COUNT];

void attacker(void) {
    // prepare cache-based transmission channel
    transmit_t transmit;
    transmit_prepare(&transmit);

    // allocate target and write the secret to it
    volatile uintptr_t *target = (volatile void *)mmap_pages(1);
    *target = SECRET;

    // take SAMPLE_COUNT samples
    for (size_t i = 0; i < SAMPLE_COUNT; i++) {

        // encode secret in transmission channel
        leak_and_transmit(&transmit, target);

        // perform cyclecount_action on all allocated cachelines
        for (size_t j = 0; j < 256; j++) {
            // perturb index to confuse prefetcher
            size_t k = (j + 5 * i) % 256;

            cyclecount_t time = cyclecount_action(transmit.probe_arr + k);

            // record latency for the known secret value
            if (k == (uint8_t)SECRET)
                samples_cached[i] = time;
            // record latency for any other value
            if (k == 0xf0)
                samples_uncached[i] = time;
        }
    }

    // print all measured latencies
    for (size_t i = 0; i < SAMPLE_COUNT; i++) {
        printf("%lu,%lu\n", samples_cached[i], samples_uncached[i]);
    }
}
