#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m16/alu.h"
#include "m16/instruction.h"
#include "m16/machine.h"

static bool current_test_failed = false;
static unsigned tests_run = 0;
static unsigned tests_failed = 0;

#define FAIL(...)                                                                                  \
    do {                                                                                           \
        fprintf(stderr, "\n  %s:%d: ", __FILE__, __LINE__);                                        \
        fprintf(stderr, __VA_ARGS__);                                                              \
        fputc('\n', stderr);                                                                       \
        current_test_failed = true;                                                                \
    } while (0)

#define CHECK_EQ_U16(expected, actual)                                                             \
    do {                                                                                           \
        const uint16_t expected_value = (uint16_t)(expected);                                      \
        const uint16_t actual_value = (uint16_t)(actual);                                          \
                                                                                                   \
        if (expected_value != actual_value) {                                                      \
            FAIL("expected 0x%04" PRIx16 ", got 0x%04" PRIx16, expected_value, actual_value);      \
        }                                                                                          \
    } while (0)

#define CHECK_EQ_U64(expected, actual)                                                             \
    do {                                                                                           \
        const uint64_t expected_value = (uint64_t)(expected);                                      \
        const uint64_t actual_value = (uint64_t)(actual);                                          \
                                                                                                   \
        if (expected_value != actual_value) {                                                      \
            FAIL("expected %" PRIu64 ", got %" PRIu64, expected_value, actual_value);              \
        }                                                                                          \
    } while (0)

#define CHECK_FALSE(value)                                                                         \
    do {                                                                                           \
        if ((value)) {                                                                             \
            FAIL("expected false, got true");                                                      \
        }                                                                                          \
    } while (0)

#define CHECK_EQ_INT(expected, actual)                                                             \
    do {                                                                                           \
        const int expected_value = (int)(expected);                                                \
        const int actual_value = (int)(actual);                                                    \
                                                                                                   \
        if (expected_value != actual_value) {                                                      \
            FAIL("expected %d, got %d", expected_value, actual_value);                             \
        }                                                                                          \
    } while (0)

#define CHECK_TRUE(value)                                                                          \
    do {                                                                                           \
        if (!(value)) {                                                                            \
            FAIL("expected true, got false");                                                      \
        }                                                                                          \
    } while (0)

static void test_init_initializes_machine(void)
{
    M16Machine machine;

    /*
     * Wypełniamy strukturę wartością inną niż zero.
     * Dzięki temu test wykryje pola, których funkcja
     * inicjalizująca zapomniała ustawić.
     */
    memset(&machine, 0xA5, sizeof(machine));

    m16_machine_power_on(&machine);

    for (size_t i = 0; i < M16_REGISTER_COUNT; ++i) {
        CHECK_EQ_U16(0, machine.cpu.r[i]);
    }

    CHECK_EQ_U16(M16_RESET_ADDRESS, machine.cpu.pc);
    CHECK_EQ_U16(M16_STACK_TOP, machine.cpu.sp);
    CHECK_EQ_U16(0, machine.cpu.flags);
    CHECK_FALSE(machine.cpu.halted);
    CHECK_EQ_U64(0, machine.cpu.cycles);

    /*
     * Licznik pętli nie może być 16-bitowy.
     *
     * Gdyby address był typu uint16_t, po 0xFFFF
     * zawinąłby się do 0 i pętla nigdy by się nie
     * zakończyła.
     */
    for (uint32_t address = 0; address < M16_ADDRESS_SPACE_WORDS; ++address) {
        const m16_word_t value = m16_bus_read(&machine.bus, (m16_addr_t)address);

        if (value != 0) {
            FAIL("memory at 0x%04" PRIx32 " was not cleared", address);
            break;
        }
    }
}

static void test_cpu_reset_preserves_memory(void)
{
    M16Machine machine;

    m16_machine_power_on(&machine);

    m16_bus_write(&machine.bus, UINT16_C(0x1234), UINT16_C(0xBEEF));

    machine.cpu.r[3] = UINT16_C(0xCAFE);
    machine.cpu.pc = UINT16_C(0x1111);
    machine.cpu.sp = UINT16_C(0x2222);
    machine.cpu.flags = UINT16_C(0xFFFF);
    machine.cpu.halted = true;
    machine.cpu.cycles = UINT64_C(123456);

    m16_machine_reset(&machine);

    CHECK_EQ_U16(UINT16_C(0xBEEF), m16_bus_read(&machine.bus, UINT16_C(0x1234)));

    CHECK_EQ_U16(0, machine.cpu.r[3]);
    CHECK_EQ_U16(M16_RESET_ADDRESS, machine.cpu.pc);
    CHECK_EQ_U16(M16_STACK_TOP, machine.cpu.sp);
    CHECK_EQ_U16(0, machine.cpu.flags);
    CHECK_FALSE(machine.cpu.halted);
    CHECK_EQ_U64(0, machine.cpu.cycles);
}

static void test_bus_round_trip_in_ram(void)
{
    M16Bus bus;

    m16_bus_init(&bus);

    const m16_addr_t addresses[] = {
        UINT16_C(0x0000),
        UINT16_C(0x0001),
        UINT16_C(0x1234),
        UINT16_C(0x7FFF),
    };

    const m16_word_t values[] = {
        UINT16_C(0x0000),
        UINT16_C(0x0001),
        UINT16_C(0xBEEF),
        UINT16_C(0xFFFF),
    };

    const size_t count = sizeof(addresses) / sizeof(addresses[0]);

    for (size_t i = 0; i < count; ++i) {
        m16_bus_write(&bus, addresses[i], values[i]);

        CHECK_EQ_U16(values[i], m16_bus_read(&bus, addresses[i]));
    }
}

static void test_instruction_encoding(void)
{
    const m16_word_t nop = m16_instruction_encode(M16_OPCODE_NOP, 0);
    const m16_word_t halt = m16_instruction_encode(M16_OPCODE_HALT, 0);
    const m16_word_t sample = m16_instruction_encode(M16_OPCODE_HALT, UINT8_C(0xA5));
    CHECK_EQ_U16(UINT16_C(0x0000), nop);
    CHECK_EQ_U16(UINT16_C(0x0100), halt);
    CHECK_EQ_U16(UINT16_C(0x01A5), sample);
    CHECK_EQ_U16(M16_OPCODE_HALT, m16_instruction_opcode(sample));
    CHECK_EQ_U16(UINT8_C(0xA5), m16_instruction_operand_byte(sample));
}

static void test_nop_advances_program_counter(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[0] = UINT16_C(0x1234);
    machine.cpu.flags = UINT16_C(0x000F);
    m16_bus_write(&machine.bus, M16_RESET_ADDRESS, m16_instruction_encode(M16_OPCODE_NOP, 0));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_OK, result);
    CHECK_EQ_U16(UINT16_C(0x8001), machine.cpu.pc);
    CHECK_EQ_U64(1, machine.cpu.cycles);
    CHECK_FALSE(machine.cpu.halted);
    CHECK_EQ_U16(UINT16_C(0x1234), machine.cpu.r[0]);
    CHECK_EQ_U16(UINT16_C(0x000F), machine.cpu.flags);
}

static void test_halt_stops_cpu(void)
{
    M16Machine machine;

    m16_machine_power_on(&machine);

    m16_bus_write(&machine.bus, M16_RESET_ADDRESS, m16_instruction_encode(M16_OPCODE_HALT, 0));

    const M16StepResult first_result = m16_machine_step(&machine);

    CHECK_EQ_INT(M16_STEP_HALTED, first_result);

    CHECK_EQ_U16(UINT16_C(0x8001), machine.cpu.pc);

    CHECK_EQ_U64(1, machine.cpu.cycles);

    CHECK_TRUE(machine.cpu.halted);

    const M16StepResult second_result = m16_machine_step(&machine);

    CHECK_EQ_INT(M16_STEP_HALTED, second_result);

    CHECK_EQ_U16(UINT16_C(0x8001), machine.cpu.pc);

    CHECK_EQ_U64(1, machine.cpu.cycles);

    CHECK_TRUE(machine.cpu.halted);
}

static void test_program_runs_until_halt(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    m16_bus_write(&machine.bus, UINT16_C(0x8000), m16_instruction_encode(M16_OPCODE_NOP, 0));
    m16_bus_write(&machine.bus, UINT16_C(0x8001), m16_instruction_encode(M16_OPCODE_NOP, 0));
    m16_bus_write(&machine.bus, UINT16_C(0x8002), m16_instruction_encode(M16_OPCODE_HALT, 0));
    M16StepResult result;
    do {
        result = m16_machine_step(&machine);
    } while (result == M16_STEP_OK);
    CHECK_EQ_INT(M16_STEP_HALTED, result);
    CHECK_EQ_U16(UINT16_C(0x8003), machine.cpu.pc);
    CHECK_EQ_U64(3, machine.cpu.cycles);
    CHECK_TRUE(machine.cpu.halted);
}

static void test_illegal_instruction_preserves_cpu_state(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    m16_bus_write(&machine.bus, M16_RESET_ADDRESS, UINT16_C(0xFF00));
    machine.cpu.r[2] = UINT16_C(0xCAFE);
    machine.cpu.flags = UINT16_C(0x0005);
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_ILLEGAL_INSTRUCTION, result);
    CHECK_EQ_U16(M16_RESET_ADDRESS, machine.cpu.pc);
    CHECK_EQ_U64(0, machine.cpu.cycles);
    CHECK_FALSE(machine.cpu.halted);
    CHECK_EQ_U16(UINT16_C(0xCAFE), machine.cpu.r[2]);
    CHECK_EQ_U16(UINT16_C(0x0005), machine.cpu.flags);
}

static void test_program_counter_wraps_after_ffff(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.pc = UINT16_C(0xFFFF);
    m16_bus_write(&machine.bus, UINT16_C(0xFFFF), m16_instruction_encode(M16_OPCODE_NOP, 0));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_OK, result);
    CHECK_EQ_U16(UINT16_C(0x0000), machine.cpu.pc);
    CHECK_EQ_U64(1, machine.cpu.cycles);
}

static void test_register_operand_encoding(void)
{
    const uint8_t mov_operand = m16_operand_encode_registers(M16_R5, M16_R2);
    const uint8_t ldi_operand = m16_operand_encode_destination(M16_R2);
    CHECK_EQ_U16(UINT8_C(0x52), mov_operand);
    CHECK_EQ_U16(UINT8_C(0x20), ldi_operand);
    CHECK_EQ_U16(M16_R5, m16_operand_destination_register(mov_operand));
    CHECK_EQ_U16(M16_R2, m16_operand_source_register(mov_operand));
}

static void test_ldi_loads_immediate_value(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[2] = UINT16_C(0x1111);
    machine.cpu.flags = UINT16_C(0x000F);
    m16_bus_write(&machine.bus,
                  UINT16_C(0x8000),
                  m16_instruction_encode(M16_OPCODE_LDI, m16_operand_encode_destination(M16_R2)));
    m16_bus_write(&machine.bus, UINT16_C(0x8001), UINT16_C(0xBEEF));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_OK, result);
    CHECK_EQ_U16(UINT16_C(0xBEEF), machine.cpu.r[2]);
    CHECK_EQ_U16(UINT16_C(0x8002), machine.cpu.pc);
    CHECK_EQ_U64(1, machine.cpu.cycles);
    CHECK_EQ_U16(UINT16_C(0x000F), machine.cpu.flags);
    CHECK_FALSE(machine.cpu.halted);
}

static void test_mov_copies_register(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[2] = UINT16_C(0xBEEF);
    machine.cpu.r[5] = UINT16_C(0x1111);
    machine.cpu.flags = UINT16_C(0x000A);
    m16_bus_write(
        &machine.bus,
        M16_RESET_ADDRESS,
        m16_instruction_encode(M16_OPCODE_MOV, m16_operand_encode_registers(M16_R5, M16_R2)));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_OK, result);
    CHECK_EQ_U16(UINT16_C(0xBEEF), machine.cpu.r[5]);
    CHECK_EQ_U16(UINT16_C(0xBEEF), machine.cpu.r[2]);
    CHECK_EQ_U16(UINT16_C(0x8001), machine.cpu.pc);
    CHECK_EQ_U64(1, machine.cpu.cycles);
    CHECK_EQ_U16(UINT16_C(0x000A), machine.cpu.flags);
}

static void test_register_program_runs_until_halt(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    m16_bus_write(&machine.bus,
                  UINT16_C(0x8000),
                  m16_instruction_encode(M16_OPCODE_LDI, m16_operand_encode_destination(M16_R2)));
    m16_bus_write(&machine.bus, UINT16_C(0x8001), UINT16_C(0xBEEF));
    m16_bus_write(
        &machine.bus,
        UINT16_C(0x8002),
        m16_instruction_encode(M16_OPCODE_MOV, m16_operand_encode_registers(M16_R5, M16_R2)));
    m16_bus_write(&machine.bus, UINT16_C(0x8003), m16_instruction_encode(M16_OPCODE_HALT, 0));
    M16StepResult result;
    do {
        result = m16_machine_step(&machine);
    } while (result == M16_STEP_OK);
    CHECK_EQ_INT(M16_STEP_HALTED, result);
    CHECK_EQ_U16(UINT16_C(0xBEEF), machine.cpu.r[2]);
    CHECK_EQ_U16(UINT16_C(0xBEEF), machine.cpu.r[5]);
    CHECK_EQ_U16(UINT16_C(0x8004), machine.cpu.pc);
    CHECK_EQ_U64(3, machine.cpu.cycles);
    CHECK_TRUE(machine.cpu.halted);
}

static void test_ldi_rejects_reserved_bits(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[2] = UINT16_C(0x1234);
    machine.cpu.flags = UINT16_C(0x0005);
    m16_bus_write(&machine.bus, M16_RESET_ADDRESS, UINT16_C(0x0221));
    m16_bus_write(&machine.bus, UINT16_C(0x8001), UINT16_C(0xBEEF));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_ILLEGAL_INSTRUCTION, result);
    CHECK_EQ_U16(UINT16_C(0x1234), machine.cpu.r[2]);
    CHECK_EQ_U16(M16_RESET_ADDRESS, machine.cpu.pc);
    CHECK_EQ_U64(0, machine.cpu.cycles);
    CHECK_EQ_U16(UINT16_C(0x0005), machine.cpu.flags);
    CHECK_FALSE(machine.cpu.halted);
}

static void test_mov_rejects_invalid_register(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[0] = UINT16_C(0x1111);
    machine.cpu.flags = UINT16_C(0x0003);
    /*
     * 0x0380:
     * opcode = 0x03, MOV
     * destination = 0x8, invalid
     * source = 0x0, R0
     */
    m16_bus_write(&machine.bus, M16_RESET_ADDRESS, UINT16_C(0x0380));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_ILLEGAL_INSTRUCTION, result);
    CHECK_EQ_U16(UINT16_C(0x1111), machine.cpu.r[0]);
    CHECK_EQ_U16(M16_RESET_ADDRESS, machine.cpu.pc);
    CHECK_EQ_U64(0, machine.cpu.cycles);
    CHECK_EQ_U16(UINT16_C(0x0003), machine.cpu.flags);
}

static void test_ldi_wraps_across_address_space(void)
{
    M16Machine machine;

    m16_machine_power_on(&machine);

    machine.cpu.pc = UINT16_C(0xFFFF);

    m16_bus_write(&machine.bus,
                  UINT16_C(0xFFFF),
                  m16_instruction_encode(M16_OPCODE_LDI, m16_operand_encode_destination(M16_R1)));

    m16_bus_write(&machine.bus, UINT16_C(0x0000), UINT16_C(0xCAFE));

    const M16StepResult result = m16_machine_step(&machine);

    CHECK_EQ_INT(M16_STEP_OK, result);

    CHECK_EQ_U16(UINT16_C(0xCAFE), machine.cpu.r[1]);

    CHECK_EQ_U16(UINT16_C(0x0001), machine.cpu.pc);

    CHECK_EQ_U64(1, machine.cpu.cycles);
}

static void test_alu_add_and_sub(void)
{
    CHECK_EQ_U16(UINT16_C(0x1235), m16_alu_add(UINT16_C(0x1234), UINT16_C(0x0001)));
    CHECK_EQ_U16(UINT16_C(0x0000), m16_alu_add(UINT16_C(0xFFFF), UINT16_C(0x0001)));
    CHECK_EQ_U16(UINT16_C(0x0FFF), m16_alu_sub(UINT16_C(0x1000), UINT16_C(0x0001)));
    CHECK_EQ_U16(UINT16_C(0xFFFF), m16_alu_sub(UINT16_C(0x0000), UINT16_C(0x0001)));
}

static void test_alu_inc_and_dec(void)
{
    CHECK_EQ_U16(UINT16_C(0x0002), m16_alu_inc(UINT16_C(0x0001)));
    CHECK_EQ_U16(UINT16_C(0x0000), m16_alu_inc(UINT16_C(0xFFFF)));
    CHECK_EQ_U16(UINT16_C(0x0000), m16_alu_dec(UINT16_C(0x0001)));
    CHECK_EQ_U16(UINT16_C(0xFFFF), m16_alu_dec(UINT16_C(0x0000)));
}

static void test_add_updates_destination(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[1] = UINT16_C(0x1234);
    machine.cpu.r[2] = UINT16_C(0x0102);
    machine.cpu.flags = UINT16_C(0x000F);
    m16_bus_write(
        &machine.bus,
        M16_RESET_ADDRESS,
        m16_instruction_encode(M16_OPCODE_ADD, m16_operand_encode_registers(M16_R1, M16_R2)));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_OK, result);
    CHECK_EQ_U16(UINT16_C(0x1336), machine.cpu.r[1]);
    CHECK_EQ_U16(UINT16_C(0x0102), machine.cpu.r[2]);
    CHECK_EQ_U16(UINT16_C(0x000F), machine.cpu.flags);
    CHECK_EQ_U16(UINT16_C(0x8001), machine.cpu.pc);
    CHECK_EQ_U64(1, machine.cpu.cycles);
    CHECK_FALSE(machine.cpu.halted);
}

static void test_sub_updates_destination(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[3] = UINT16_C(0x1000);
    machine.cpu.r[4] = UINT16_C(0x0001);
    machine.cpu.flags = UINT16_C(0x000A);
    m16_bus_write(
        &machine.bus,
        M16_RESET_ADDRESS,
        m16_instruction_encode(M16_OPCODE_SUB, m16_operand_encode_registers(M16_R3, M16_R4)));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_OK, result);
    CHECK_EQ_U16(UINT16_C(0x0FFF), machine.cpu.r[3]);
    CHECK_EQ_U16(UINT16_C(0x0001), machine.cpu.r[4]);
    CHECK_EQ_U16(UINT16_C(0x000A), machine.cpu.flags);
    CHECK_EQ_U16(UINT16_C(0x8001), machine.cpu.pc);
    CHECK_EQ_U64(1, machine.cpu.cycles);
}

static void test_inc_wraps_at_ffff(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[5] = UINT16_C(0xFFFF);
    m16_bus_write(&machine.bus,
                  M16_RESET_ADDRESS,
                  m16_instruction_encode(M16_OPCODE_INC, m16_operand_encode_destination(M16_R5)));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_OK, result);
    CHECK_EQ_U16(UINT16_C(0x0000), machine.cpu.r[5]);
    CHECK_EQ_U16(UINT16_C(0x8001), machine.cpu.pc);
    CHECK_EQ_U64(1, machine.cpu.cycles);
}

static void test_dec_wraps_at_zero(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[6] = UINT16_C(0x0000);
    m16_bus_write(&machine.bus,
                  M16_RESET_ADDRESS,
                  m16_instruction_encode(M16_OPCODE_DEC, m16_operand_encode_destination(M16_R6)));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_OK, result);
    CHECK_EQ_U16(UINT16_C(0xFFFF), machine.cpu.r[6]);
    CHECK_EQ_U16(UINT16_C(0x8001), machine.cpu.pc);
    CHECK_EQ_U64(1, machine.cpu.cycles);
}

static void test_arithmetic_program_runs_until_halt(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    m16_bus_write(&machine.bus,
                  UINT16_C(0x8000),
                  m16_instruction_encode(M16_OPCODE_LDI, m16_operand_encode_destination(M16_R0)));
    m16_bus_write(&machine.bus, UINT16_C(0x8001), UINT16_C(10));
    m16_bus_write(&machine.bus,
                  UINT16_C(0x8002),
                  m16_instruction_encode(M16_OPCODE_LDI, m16_operand_encode_destination(M16_R1)));
    m16_bus_write(&machine.bus, UINT16_C(0x8003), UINT16_C(3));
    m16_bus_write(
        &machine.bus,
        UINT16_C(0x8004),
        m16_instruction_encode(M16_OPCODE_ADD, m16_operand_encode_registers(M16_R0, M16_R1)));
    m16_bus_write(&machine.bus,
                  UINT16_C(0x8005),
                  m16_instruction_encode(M16_OPCODE_INC, m16_operand_encode_destination(M16_R0)));
    m16_bus_write(
        &machine.bus,
        UINT16_C(0x8006),
        m16_instruction_encode(M16_OPCODE_SUB, m16_operand_encode_registers(M16_R0, M16_R1)));
    m16_bus_write(&machine.bus,
                  UINT16_C(0x8007),
                  m16_instruction_encode(M16_OPCODE_DEC, m16_operand_encode_destination(M16_R0)));
    m16_bus_write(&machine.bus, UINT16_C(0x8008), m16_instruction_encode(M16_OPCODE_HALT, 0));
    M16StepResult result;
    do {
        result = m16_machine_step(&machine);
    } while (result == M16_STEP_OK);
    CHECK_EQ_INT(M16_STEP_HALTED, result);
    CHECK_EQ_U16(UINT16_C(10), machine.cpu.r[0]);
    CHECK_EQ_U16(UINT16_C(3), machine.cpu.r[1]);
    CHECK_EQ_U16(UINT16_C(0x8009), machine.cpu.pc);
    CHECK_EQ_U64(7, machine.cpu.cycles);
    CHECK_TRUE(machine.cpu.halted);
}

static void test_add_rejects_invalid_register(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[0] = UINT16_C(0xAAAA);
    machine.cpu.flags = UINT16_C(0x0007);
    /*
     * 0x0480:
     *
     * opcode = 0x04, ADD
     * destination = 0x8, invalid
     * source = 0x0, R0
     */
    m16_bus_write(&machine.bus, M16_RESET_ADDRESS, UINT16_C(0x0480));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_ILLEGAL_INSTRUCTION, result);
    CHECK_EQ_U16(UINT16_C(0xAAAA), machine.cpu.r[0]);
    CHECK_EQ_U16(M16_RESET_ADDRESS, machine.cpu.pc);
    CHECK_EQ_U64(0, machine.cpu.cycles);
    CHECK_EQ_U16(UINT16_C(0x0007), machine.cpu.flags);
}

static void test_inc_rejects_reserved_bits(void)
{
    M16Machine machine;
    m16_machine_power_on(&machine);
    machine.cpu.r[1] = UINT16_C(0x1234);
    machine.cpu.flags = UINT16_C(0x0005); /* * 0x0611: * * opcode = 0x06, INC * destination = 0x1,
                                             R1 * reserved bits = 0x1, invalid */
    m16_bus_write(&machine.bus, M16_RESET_ADDRESS, UINT16_C(0x0611));
    const M16StepResult result = m16_machine_step(&machine);
    CHECK_EQ_INT(M16_STEP_ILLEGAL_INSTRUCTION, result);
    CHECK_EQ_U16(UINT16_C(0x1234), machine.cpu.r[1]);
    CHECK_EQ_U16(M16_RESET_ADDRESS, machine.cpu.pc);
    CHECK_EQ_U64(0, machine.cpu.cycles);
    CHECK_EQ_U16(UINT16_C(0x0005), machine.cpu.flags);
}

static void run_test(const char *name, void (*test_function)(void))
{
    ++tests_run;
    current_test_failed = false;

    printf("%-40s", name);
    fflush(stdout);

    test_function();

    if (current_test_failed) {
        ++tests_failed;
        puts("FAIL");
    } else {
        puts("PASS");
    }
}

int main(void)
{
    run_test("init initializes machine", test_init_initializes_machine);

    run_test("CPU reset preserves memory", test_cpu_reset_preserves_memory);

    run_test("bus read/write round trip", test_bus_round_trip_in_ram);

    run_test("instruction encoding", test_instruction_encoding);

    run_test("NOP advances program counter", test_nop_advances_program_counter);

    run_test("HALT stops CPU", test_halt_stops_cpu);

    run_test("program runs until HALT", test_program_runs_until_halt);

    run_test("illegal instruction preserves state", test_illegal_instruction_preserves_cpu_state);

    run_test("program counter wraps after FFFF", test_program_counter_wraps_after_ffff);

    run_test("register operand encoding", test_register_operand_encoding);

    run_test("LDI loads immediate value", test_ldi_loads_immediate_value);

    run_test("MOV copies register", test_mov_copies_register);

    run_test("register program runs until HALT", test_register_program_runs_until_halt);

    run_test("LDI rejects reserved bits", test_ldi_rejects_reserved_bits);

    run_test("MOV rejects invalid register", test_mov_rejects_invalid_register);

    run_test("LDI wraps across address space", test_ldi_wraps_across_address_space);

    run_test("ALU add and sub", test_alu_add_and_sub);

    run_test("ALU inc and dec", test_alu_inc_and_dec);

    run_test("ADD updates destination", test_add_updates_destination);

    run_test("SUB updates destination", test_sub_updates_destination);

    run_test("INC wraps at FFFF", test_inc_wraps_at_ffff);

    run_test("DEC wraps at zero", test_dec_wraps_at_zero);

    run_test("arithmetic program runs until HALT", test_arithmetic_program_runs_until_halt);

    run_test("ADD rejects invalid register", test_add_rejects_invalid_register);

    run_test("INC rejects reserved bits", test_inc_rejects_reserved_bits);

    printf("\n%u tests, %u failed\n", tests_run, tests_failed);

    return tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}