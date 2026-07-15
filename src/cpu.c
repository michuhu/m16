#include "m16/cpu.h"

#include <assert.h>
#include <string.h>

void m16_cpu_reset(M16Cpu *cpu) {
    assert(cpu != NULL);

    memset(cpu, 0, sizeof(*cpu));

    cpu->pc = M16_RESET_ADDRESS;
    cpu->sp = M16_STACK_TOP;
}

