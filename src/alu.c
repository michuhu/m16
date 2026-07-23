#include "m16/alu.h"

m16_word_t m16_alu_add(m16_word_t left, m16_word_t right)
{
    return (m16_word_t)(left + right);
}

m16_word_t m16_alu_sub(m16_word_t left, m16_word_t right)
{
    return (m16_word_t)(left - right);
}

m16_word_t m16_alu_inc(m16_word_t value)
{
    return m16_alu_add(value, UINT16_C(1));
}

m16_word_t m16_alu_dec(m16_word_t value)
{
    return m16_alu_sub(value, UINT16_C(1));
}