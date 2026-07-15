#ifndef MS16_TYPES_H
#define MS16_TYPES_H

#include <stdint.h>
#include <limits.h>

typedef uint16_t m16_word_t;
typedef uint16_t m16_addr_t;

enum {
    M16_REGISTER_COUNT = 8,
    M16_ADDRESS_SPACE_WORDS = 1u << 16
};

#define M16_RESET_ADDRESS   UINT16_C(0x8000)
#define M16_STACK_TOP       UINT16_C(0x8000)

_Static_assert(CHAR_BIT == 8, "M16 requires 8-bit host bytes");

_Static_assert(sizeof(m16_word_t) * CHAR_BIT == 16, "m16_word_t must be exactly 16 bits");

_Static_assert(sizeof(m16_addr_t) * CHAR_BIT == 16, "m16_addr_t must be exactly 16 bits");  

#endif