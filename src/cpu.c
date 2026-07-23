#include "m16/alu.h"
#include "m16/cpu.h"
#include "m16/instruction.h"

#include <assert.h>
#include <string.h>

static void m16_cpu_complete_instruction(M16Cpu *cpu, uint16_t instruction_words)
{
    cpu->pc = (m16_addr_t)(cpu->pc + instruction_words);

    ++cpu->cycles;
}

void m16_cpu_reset(M16Cpu *cpu)
{
    assert(cpu != NULL);

    memset(cpu, 0, sizeof(*cpu));

    cpu->pc = M16_RESET_ADDRESS;
    cpu->sp = M16_STACK_TOP;
}

static bool
m16_cpu_decode_binary_registers(uint8_t operand_byte, uint8_t *destination, uint8_t *source)
{
    *destination = m16_operand_destination_register(operand_byte);
    *source = m16_operand_source_register(operand_byte);
    return m16_register_number_is_valid(*destination) && m16_register_number_is_valid(*source);
}

static bool m16_decode_unary_register(uint8_t operand_byte, uint8_t *destination)
{
    *destination = m16_operand_destination_register(operand_byte);
    const uint8_t reserved_bits = (uint8_t)(operand_byte & M16_LOW_NIBBLE_MASK);

    return m16_register_number_is_valid(*destination) && reserved_bits == 0;
}

M16StepResult m16_cpu_step(M16Cpu *cpu, M16Bus *bus)
{
    assert(cpu != NULL);
    assert(bus != NULL);

    if (cpu->halted) {
        return M16_STEP_HALTED;
    }

    const m16_word_t instruction = m16_bus_read(bus, cpu->pc);

    const uint8_t opcode = m16_instruction_opcode(instruction);

    const uint8_t operand_byte = m16_instruction_operand_byte(instruction);

    switch (opcode) {
        case M16_OPCODE_NOP:
            if (operand_byte != 0) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            m16_cpu_complete_instruction(cpu, 1);
            return M16_STEP_OK;

        case M16_OPCODE_HALT:
            if (operand_byte != 0) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            m16_cpu_complete_instruction(cpu, 1);

            cpu->halted = true;

            return M16_STEP_HALTED;

        case M16_OPCODE_LDI: {
            uint8_t destination;

            if (!m16_decode_unary_register(operand_byte, &destination)) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            const m16_addr_t immediate_address = (m16_addr_t)(cpu->pc + UINT16_C(1));

            const m16_word_t immediate = m16_bus_read(bus, immediate_address);

            cpu->r[destination] = immediate;
            m16_cpu_complete_instruction(cpu, 2);
            return M16_STEP_OK;
        }

        case M16_OPCODE_MOV: {
            uint8_t destination, source;

            if (!m16_cpu_decode_binary_registers(operand_byte, &destination, &source)) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            cpu->r[destination] = cpu->r[source];
            m16_cpu_complete_instruction(cpu, 1);
            return M16_STEP_OK;
        }

        case M16_OPCODE_ADD: {
            uint8_t destination, source;

            if (!m16_cpu_decode_binary_registers(operand_byte, &destination, &source)) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            cpu->r[destination] = m16_alu_add(cpu->r[destination], cpu->r[source]);
            m16_cpu_complete_instruction(cpu, 1);
            return M16_STEP_OK;
        }

        case M16_OPCODE_SUB: {
            uint8_t destination, source;

            if (!m16_cpu_decode_binary_registers(operand_byte, &destination, &source)) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            cpu->r[destination] = m16_alu_sub(cpu->r[destination], cpu->r[source]);
            m16_cpu_complete_instruction(cpu, 1);
            return M16_STEP_OK;
        }

        case M16_OPCODE_INC: {
            uint8_t destination;

            if (!m16_decode_unary_register(operand_byte, &destination)) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            cpu->r[destination] = m16_alu_inc(cpu->r[destination]);
            m16_cpu_complete_instruction(cpu, 1);
            return M16_STEP_OK;
        }

        case M16_OPCODE_DEC: {
            uint8_t destination;

            if (!m16_decode_unary_register(operand_byte, &destination)) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            cpu->r[destination] = m16_alu_dec(cpu->r[destination]);
            m16_cpu_complete_instruction(cpu, 1);
            return M16_STEP_OK;
        }

        default:
            return M16_STEP_ILLEGAL_INSTRUCTION;
    }
}