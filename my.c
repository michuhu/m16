#include "m16/instruction.h"
#include <stdio.h>

void print_ldi_instruction(uint8_t operand_byte)
{
    const uint8_t d = m16_operand_destination_register(operand_byte);
    const uint8_t s = m16_operand_source_register(operand_byte);

    printf("LDI instruction: destination register = R%u, source register = R%u\n", d, s);
}

int main(void)
{
    uint8_t operand_byte = m16_operand_encode_registers(M16_R2, M16_R3);
    printf("Encoded operand byte: 0x%02X\n", operand_byte);

    print_ldi_instruction(operand_byte);

    return 0;
}