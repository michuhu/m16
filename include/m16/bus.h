#ifndef M16_BUS_H
#define M16_BUS_H

#include "m16/types.h"

typedef struct {
    m16_word_t words[M16_ADDRESS_SPACE_WORDS];
} M16Bus;

void m16_bus_init(M16Bus *bus);

m16_word_t m16_bus_read(const M16Bus *bus, m16_addr_t address);

void m16_bus_write(M16Bus *bus, m16_addr_t address, m16_word_t value);

#endif // M16_BUS_H
