# M16 Architecture

Version: 0.1  
Status: draft

## 1. Machine word

M16 is a 16-bit architecture.

A machine word contains exactly 16 bits.

All arithmetic results are reduced modulo 2^16.

Examples:

- 0xFFFF + 1 = 0x0000
- 0x0000 - 1 = 0xFFFF

## 2. Addressing

Addresses contain 16 bits.

The address space contains 65,536 word addresses:

- first address: 0x0000
- last address: 0xFFFF

Memory is word-addressed. Each address identifies one 16-bit word.

## 3. Registers

The CPU contains eight general-purpose registers:

- R0
- R1
- R2
- R3
- R4
- R5
- R6
- R7

The CPU also contains:

- PC: program counter
- SP: stack pointer
- FLAGS: status flags

## 4. Flags

The following flag bits are reserved:

- bit 0: Z, zero
- bit 1: N, negative
- bit 2: C, carry
- bit 3: V, signed overflow

All remaining flag bits are reserved and must be zero after reset.

## 5. Reset state

After CPU reset:

- R0-R7 = 0x0000
- PC = 0x8000
- SP = 0x8000
- FLAGS = 0x0000
- halted = false
- cycle counter = 0

CPU reset does not modify memory.

## 6. Initial memory map

The provisional memory map is:

- 0x0000-0x7FFF: RAM
- 0x8000-0xFEFF: ROM
- 0xFF00-0xFFFF: memory-mapped I/O

Memory protection and I/O decoding are not implemented in version 0.1.

## 7. Stack

The stack grows toward lower addresses.

The initial value of SP is 0x8000.

A push operation will eventually use the following semantics:

1. SP = SP - 1
2. memory[SP] = value

A pop operation will eventually use:

1. value = memory[SP]
2. SP = SP + 1

Stack instructions are not implemented in version 0.1.

## 8. Determinism

Given the same initial CPU state, memory state and input device state,
the machine must always produce the same result.

The emulator must not depend on uninitialized host memory.