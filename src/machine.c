#include "m16/machine.h"

#include <assert.h>
#include <stddef.h>

void m16_machine_init(M16Machine *machine) 
{
    assert(machine != NULL);

    m16_bus_init(&machine->bus);
    m16_cpu_reset(&machine->cpu);
}

void m16_machine_reset(M16Machine *machine)
{
    assert (machine != NULL);

    m16_cpu_reset(&machine->cpu);
}