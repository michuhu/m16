#ifndef M16_CPU_H
#define M16_CPU_H

#include <stdint.h>
#include <stdbool.h>

#include "m16/types.h"
#include "m16/bus.h"

typedef enum {
    M16_FLAG_Z = 1u << 0, // Zero flag
    M16_FLAG_N = 1u << 1, // Negative flag
    M16_FLAG_C = 1u << 2, // Carry flag
    M16_FLAG_V = 1u << 3  // Overflow flag
} M16Flag;

typedef enum {
    M16_STEP_OK,
    M16_STEP_HALTED,
    M16_STEP_ILLEGAL_INSTRUCTION,
} M16StepResult;

typedef struct {
    m16_word_t r[M16_REGISTER_COUNT];

    m16_addr_t pc; // Program counter
    m16_addr_t sp; // Stack pointer

    m16_word_t flags; // CPU flags

    bool halted;     // CPU halted state
    uint64_t cycles; // Number of cycles executed
} M16Cpu;

void m16_cpu_reset(M16Cpu *cpu);

M16StepResult m16_cpu_step(M16Cpu *cpu, M16Bus *bus);

#endif // M16_CPU_H