#ifndef M16_INSTRUCTION_H
#define M16_INSTRUCTION_H

#include <stdint.h>

#include "m16/types.h"

typedef enum {
    M16_OPCODE_NOP = 0x00,
    M16_OPCODE_HALT = 0x01,
} M16Opcode;

enum {
    M16_OPCODE_SHIFT = 8,
};

#define M16_OPERAND_BYTE_MASK UINT16_C(0x00FF)

static inline m16_word_t m16_instruction_encode(M16Opcode opcode, uint8_t operand_byte)
{
    const m16_word_t encoded_opcode = (m16_word_t)((m16_word_t)(uint8_t)opcode << M16_OPCODE_SHIFT);

    return (m16_word_t)(encoded_opcode | (m16_word_t)operand_byte);
}

static inline uint8_t m16_instruction_opcode(m16_word_t instruction)
{
    return (uint8_t)(instruction >> M16_OPCODE_SHIFT);
}

static inline uint8_t m16_instruction_operand_byte(m16_word_t instruction)
{
    return (uint8_t)(instruction & M16_OPERAND_BYTE_MASK);
}

#endif