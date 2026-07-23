#ifndef M16_INSTRUCTION_H
#define M16_INSTRUCTION_H

#include <stdint.h>
#include <stdbool.h>

#include "m16/types.h"

typedef enum {
    M16_OPCODE_NOP = 0x00,
    M16_OPCODE_HALT = 0x01,
    M16_OPCODE_LDI = 0x02,
    M16_OPCODE_MOV = 0x03,
    M16_OPCODE_ADD = 0x04,
    M16_OPCODE_SUB = 0x05,
    M16_OPCODE_INC = 0x06,
    M16_OPCODE_DEC = 0x07,
} M16Opcode;

typedef enum {
    M16_R0 = 0,
    M16_R1 = 1,
    M16_R2 = 2,
    M16_R3 = 3,
    M16_R4 = 4,
    M16_R5 = 5,
    M16_R6 = 6,
    M16_R7 = 7
} M16Register;

enum {
    M16_OPCODE_SHIFT = 8,
    M16_DESTINATION_REGISTER_SHIFT = 4,
};

#define M16_OPERAND_BYTE_MASK UINT16_C(0x00FF)
#define M16_REGISTER_FIELD_MASK UINT8_C(0x0F)
#define M16_LOW_NIBBLE_MASK UINT8_C(0x0F)

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

static inline uint8_t m16_operand_encode_registers(M16Register destination, M16Register source)
{
    return (uint8_t)(((uint8_t)destination << M16_DESTINATION_REGISTER_SHIFT) | (uint8_t)source);
}

static inline uint8_t m16_operand_encode_destination(M16Register destination)
{
    return (uint8_t)((uint8_t)destination << M16_DESTINATION_REGISTER_SHIFT);
}

static inline uint8_t m16_operand_destination_register(uint8_t operand_byte)
{
    return (uint8_t)((operand_byte >> M16_DESTINATION_REGISTER_SHIFT) & M16_REGISTER_FIELD_MASK);
}

static inline uint8_t m16_operand_source_register(uint8_t operand_byte)
{
    return (uint8_t)(operand_byte & M16_REGISTER_FIELD_MASK);
}

static inline bool m16_register_number_is_valid(uint8_t register_number)
{
    return register_number < M16_REGISTER_COUNT;
}

#endif