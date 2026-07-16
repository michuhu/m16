#include "m16/machine.h"

#include <assert.h>
#include <stddef.h>

void m16_machine_power_on(M16Machine *machine)
{
    assert(machine != NULL);

    m16_bus_init(&machine->bus);
    m16_cpu_reset(&machine->cpu);
}

void m16_machine_reset(M16Machine *machine)
{
    assert(machine != NULL);

    m16_cpu_reset(&machine->cpu);
}

M16StepResult m16_machine_step(M16Machine *machine)
{
    assert(machine != NULL);

    return m16_cpu_step(&machine->cpu, &machine->bus);
}