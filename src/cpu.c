#include "m16/cpu.h"
#include "m16/instruction.h"

#include <assert.h>
#include <string.h>

void m16_cpu_reset(M16Cpu *cpu)
{
    assert(cpu != NULL);

    memset(cpu, 0, sizeof(*cpu));

    cpu->pc = M16_RESET_ADDRESS;
    cpu->sp = M16_STACK_TOP;
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

            cpu->pc = (m16_addr_t)(cpu->pc + UINT16_C(1));

            ++cpu->cycles;
            return M16_STEP_OK;

        case M16_OPCODE_HALT:
            if (operand_byte != 0) {
                return M16_STEP_ILLEGAL_INSTRUCTION;
            }

            cpu->pc = (m16_addr_t)(cpu->pc + UINT16_C(1));

            ++cpu->cycles;

            cpu->halted = true;

            return M16_STEP_HALTED;

        default:
            return M16_STEP_ILLEGAL_INSTRUCTION;
    }
}