#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m16/machine.h"

static bool current_test_failed = false;
static unsigned tests_run = 0;
static unsigned tests_failed = 0;

#define FAIL(...)                                      \
    do {                                               \
        fprintf(                                       \
            stderr,                                    \
            "\n  %s:%d: ",                             \
            __FILE__,                                  \
            __LINE__                                   \
        );                                             \
        fprintf(stderr, __VA_ARGS__);                  \
        fputc('\n', stderr);                            \
        current_test_failed = true;                    \
    } while (0)

#define CHECK_EQ_U16(expected, actual)                  \
    do {                                               \
        const uint16_t expected_value =                \
            (uint16_t)(expected);                      \
        const uint16_t actual_value =                  \
            (uint16_t)(actual);                        \
                                                       \
        if (expected_value != actual_value) {          \
            FAIL(                                      \
                "expected 0x%04" PRIx16                \
                ", got 0x%04" PRIx16,                  \
                expected_value,                        \
                actual_value                           \
            );                                         \
        }                                              \
    } while (0)

#define CHECK_EQ_U64(expected, actual)                  \
    do {                                               \
        const uint64_t expected_value =                \
            (uint64_t)(expected);                      \
        const uint64_t actual_value =                  \
            (uint64_t)(actual);                        \
                                                       \
        if (expected_value != actual_value) {          \
            FAIL(                                      \
                "expected %" PRIu64                    \
                ", got %" PRIu64,                      \
                expected_value,                        \
                actual_value                           \
            );                                         \
        }                                              \
    } while (0)

#define CHECK_FALSE(value)                             \
    do {                                               \
        if ((value)) {                                 \
            FAIL("expected false, got true");          \
        }                                              \
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

    m16_machine_init(&machine);

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
    for (
        uint32_t address = 0;
        address < M16_ADDRESS_SPACE_WORDS;
        ++address
    ) {
        const m16_word_t value = m16_bus_read(
            &machine.bus,
            (m16_addr_t)address
        );

        if (value != 0) {
            FAIL(
                "memory at 0x%04" PRIx32
                " was not cleared",
                address
            );
            break;
        }
    }
}

static void test_cpu_reset_preserves_memory(void)
{
    M16Machine machine;

    m16_machine_init(&machine);

    m16_bus_write(
        &machine.bus,
        UINT16_C(0x1234),
        UINT16_C(0xBEEF)
    );

    machine.cpu.r[3] = UINT16_C(0xCAFE);
    machine.cpu.pc = UINT16_C(0x1111);
    machine.cpu.sp = UINT16_C(0x2222);
    machine.cpu.flags = UINT16_C(0xFFFF);
    machine.cpu.halted = true;
    machine.cpu.cycles = UINT64_C(123456);

    m16_machine_reset(&machine);

    CHECK_EQ_U16(
        UINT16_C(0xBEEF),
        m16_bus_read(
            &machine.bus,
            UINT16_C(0x1234)
        )
    );

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

    const size_t count =
        sizeof(addresses) / sizeof(addresses[0]);

    for (size_t i = 0; i < count; ++i) {
        m16_bus_write(
            &bus,
            addresses[i],
            values[i]
        );

        CHECK_EQ_U16(
            values[i],
            m16_bus_read(&bus, addresses[i])
        );
    }
}

static void run_test(
    const char *name,
    void (*test_function)(void)
)
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
    run_test(
        "init initializes machine",
        test_init_initializes_machine
    );

    run_test(
        "CPU reset preserves memory",
        test_cpu_reset_preserves_memory
    );

    run_test(
        "bus read/write round trip",
        test_bus_round_trip_in_ram
    );

    printf(
        "\n%u tests, %u failed\n",
        tests_run,
        tests_failed
    );

    return tests_failed == 0
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}