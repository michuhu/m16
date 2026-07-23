#ifndef M16_ALU_H
#define M16_ALU_H

#include "m16/types.h"

m16_word_t m16_alu_add(m16_word_t left, m16_word_t right);

m16_word_t m16_alu_sub(m16_word_t left, m16_word_t right);

m16_word_t m16_alu_inc(m16_word_t value);

m16_word_t m16_alu_dec(m16_word_t value);

#endif