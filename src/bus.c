#include "m16/bus.h"

#include <assert.h>
#include <string.h>

void m16_bus_init(M16Bus *bus)
{
    assert(bus != NULL);
    memset(bus->words, 0, sizeof(bus->words));
}

m16_word_t m16_bus_read(
    const M16Bus *bus,
    m16_addr_t address
)
{
    assert(bus != NULL);
    return bus->words[address];
}

void m16_bus_write(
    M16Bus *bus,
    m16_addr_t address,
    m16_word_t value
)
{
    assert(bus != NULL);
    bus->words[address] = value;
}