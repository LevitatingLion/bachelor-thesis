// interface for common functionality not specific to a particular attack

#define _GNU_SOURCE
#include <setjmp.h>
#include <signal.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// possible settings:
// VICTIM_LOAD, VICTIM_STORE: victim process performs loads or stores
// FAULT_SIGNAL, FAULT_TRANSIENT: handle faults with signal handler or avoid faults with transient execution
// CACHE_FR, CACHE_FF: use flush+reload or flush+flush as cache sidechannel attack

// number of samples to measure
// metrics are averaged over these samples
#ifndef SAMPLE_COUNT
#define SAMPLE_COUNT 100
#endif

// number of attacks to perform to leak a single value
#define COUNTS_PER_SAMPLE 200

// thresholds for flush+reload and flush+flush
#define THRESHOLD_FR 100
#define THRESHOLD_FF 124

// these logical cores should be on the same physical core
#define CPU_ATTACKER 6
#define CPU_VICTIM 2

// secret value of the victim, retrieved by the attacker
#define SECRET 0x41424344

// fixed size of a page and cacheline
#define PAGE_SIZE 0x1000
#define CACHELINE_SIZE 0x40

// commonly used function attributes
#define ALWAYS_INLINE __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define UNUSED __attribute__((unused))
#define NAKED __attribute__((naked))

// terminate if condition is false
#define ASSERT(cond)                                         \
    do {                                                     \
        if (!(cond)) {                                       \
            fprintf(stderr, "Assertion %s failed\n", #cond); \
            exit(EXIT_FAILURE);                              \
        }                                                    \
    } while (0)

// struct representing a page, properly aligned
typedef struct {
    alignas(PAGE_SIZE) uint8_t data[PAGE_SIZE];
} page_t;

// struct representing a cacheline, properly aligned
typedef struct {
    alignas(CACHELINE_SIZE) uint8_t data[CACHELINE_SIZE];
} cacheline_t;

// numer of cycles, as returned by rdtsc
typedef uint64_t cyclecount_t;

// struct used to store all data specific to the transmission channel
// and data for the measured metrics
typedef struct {
    // hit counts for each possible byte
    size_t hits[256];
    // probe array of 256 pages
    page_t *probe_arr;
    // cycles needed for leak attempts
    cyclecount_t cycles_leak;
    // cycles needed for decoding
    cyclecount_t cycles_decode;
    // start timestamp
    uint64_t start_time_ns;
    // number of correct transmissions
    size_t correct_transmissions;
    // indicates if transmit_leak_start was called
    bool timing_leak;
} transmit_t;

// saved execution state, used by the signal handler
extern jmp_buf try_setjmp_buf;
// records if a try_begin block has been entered
extern volatile sig_atomic_t try_entered;

// flush the given address using clflush
ALWAYS_INLINE void mem_flush(const volatile void *ptr);
// load from the given address
ALWAYS_INLINE void mem_access(const volatile void *ptr);

// load value from target and encode it into probe_arr
// during transient execution
void probe_access_transient(const volatile void *target, const page_t *probe_arr);
// load value from target and encode it into probe_arr
void probe_access(const volatile void *target, const page_t *probe_arr);

// return current cycle count, properly fenced for any operations
ALWAYS_INLINE cyclecount_t cyclecount(void);

// load from the given address and measure number of cycles it took
NOINLINE cyclecount_t cyclecount_access(const volatile void *ptr);
// load from the given address and measure number of cycles it took, then flush the address
NOINLINE cyclecount_t cyclecount_access_flush(const volatile void *ptr);
// flush the given address and measure number of cycles it took
NOINLINE cyclecount_t cyclecount_flush(const volatile void *ptr);

// bind current process to the given logical core
void bind_to_cpu(int cpu);

// get the current wall time in nanoseconds
uint64_t get_wall_time_ns(void);

// save current execution state and enable fault handling via a signal handler
// returns true on entry, false on abort
// is implemented as a macro, so that setjmp has the scope of the calling function
// signature would be
//   bool try_begin(void);
#define try_begin()              \
    ({                           \
        try_entered = 1;         \
        !setjmp(try_setjmp_buf); \
    })
// disable fault handling again
void try_end(void);
// trigger a fault
void try_abort(void);

// allocate a number of pages
page_t *mmap_pages(size_t count);
// page-out the given pages
void mmap_evict(page_t *pages, size_t count);

// prepare cache-based transmission channel
void transmit_prepare(transmit_t *transmit);
// access the target during transient execution, encode resulting value in transmission channel
// handle faults as specified by FAULT_* definitions
void leak_and_transmit(transmit_t *transmit, const volatile void *target);
// access probe array and record hits
// should be called after every call to leak_and_transmit
void transmit_receive(transmit_t *transmit);
// determine which byte was leaked and check if it's the secret byte
// this terminates the current sample and starts a new one
void transmit_decode(transmit_t *transmit);
// display metrics, averaged over all collected samples
void transmit_display(transmit_t *transmit);

// used to count the number of cycles required for each attack
// called before calling leak_and_transmit
void transmit_leak_start(transmit_t *transmit);
// called after calling leak_and_transmit
void transmit_leak_end(transmit_t *transmit);

#if defined(VICTIM_LOAD) || defined(VICTIM_STORE)
// perform victim operation as indicated by the VICTIM_* definitions
void victim(void);
#endif

// load secret value in a loop
void victim_load(void);
// store secret value in a loop
void victim_store(void);

// attacker function is defined in the c file for each attack
void attacker(void);
