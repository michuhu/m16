#ifndef M16_MACHINE_H
#define M16_MACHINE_H

#include "m16/cpu.h"
#include "m16/bus.h"

typedef struct {
    M16Cpu cpu;
    M16Bus bus;
} M16Machine;

void m16_machine_power_on(M16Machine *machine);
void m16_machine_reset(M16Machine *machine);

M16StepResult m16_machine_step(M16Machine *machine);

#endif