#ifndef __MACHINE_H__
#define __MACHINE_H__

#include <stdint.h>
#include "code.h"

#define WORD_SIZE_BYTES 8
#define WORD_SIZE_BITS (WORD_SIZE_BYTES * 8)
#define HALFWORD_SIZE_BITS (WORD_SIZE_BITS / 2)

#define REGISTER_NULL   0x0123456789ABCDEF

#define CONDITION_ZERO      0b00000001
#define CONDITION_NEGATIVE  0b00000010
#define CONDITION_POSITIVE  0b00000100

struct machine_t {
    uint64_t registers[32]; // 31 general purpose registers, plus an extra for the zero register
    uint64_t sp;
    uint64_t pc;
    uint64_t code_top;
    uint64_t code_bot;
    struct instruction_t *code;
    void *stack;
    uint64_t stack_top;
    uint64_t stack_bot;
    uint8_t conditions;
};

extern struct machine_t machine;

void init_machine(uint64_t sp, uint64_t pc, char *code_filepath);
void print_memory();
struct instruction_t fetch();
uint64_t get_value(struct operand_t operand);
void put_value(struct operand_t operand, uint64_t value);
uint64_t get_memory_address(struct operand_t operand);
void execute(struct instruction_t instruction);

#endif // __MACHINE_H__