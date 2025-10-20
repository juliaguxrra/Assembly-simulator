#ifndef __CODE_H__
#define __CODE_H__

#define OPERATION_add   0x00646461
#define OPERATION_sub   0x00627573
#define OPERATION_subs  0x73627573
#define OPERATION_mul   0x006C756D
#define OPERATION_sdiv  0x76696473
#define OPERATION_udiv  0x76696475
#define OPERATION_neg   0x0067656E
#define OPERATION_lsl   0x006C736C
#define OPERATION_lsr   0x0072736C
#define OPERATION_and   0x00646E61
#define OPERATION_orr   0x0072726F
#define OPERATION_eor   0x00726F65
#define OPERATION_mvn   0x006E766D
#define OPERATION_ldr   0x0072646C
#define OPERATION_str   0x00727473
#define OPERATION_mov   0x00766F6D
#define OPERATION_ret   0x00746572
#define OPERATION_b     0x00000062
#define OPERATION_bl    0x00006C62
#define OPERATION_cmp   0x00706D63
#define OPERATION_bne   0x656E2E62
#define OPERATION_beq   0x71652E62
#define OPERATION_blt   0x746C2E62
#define OPERATION_bgt   0x74672E62
#define OPERATION_ble   0x656C2E62
#define OPERATION_bge   0x65672E62
#define OPERATION_nop   0x00706F6E
#define OPERATION_clz   0x007A6C63
#define OPERATION_cls   0x00736C63
#define OPERATION_ldrb  0x6272646C
#define OPERATION_strb  0x62727473
#define OPERATION_NULL  0x0

#define OPERAND_register    'r'
#define OPERAND_constant    '#'
#define OPERAND_memory      '['
#define OPERAND_address     'a'
#define OPERAND_NULL        '\0'

#define REGISTER_w      'w'
#define REGISTER_x      'x'
#define REGISTER_sp     's'
#define REGISTER_pc     'p'

#define MAX_OPERANDS 3
#define INSTRUCTION_SIZE 4

struct operand_t {
    char type;              // OPERAND_* constants above
    char reg_type;     // Used for register and memory operands; REGISTER_* constants above
    uint8_t reg_num;   // Only used for w and x registers
    uint32_t constant;      // Used for constant, memory, and address operands
};

struct instruction_t {
    unsigned int operation;     // OPERATION_* constants above
    struct operand_t operands[MAX_OPERANDS];
};

void print_operand(struct operand_t operand);
void print_instruction(struct instruction_t instruction);
struct instruction_t *parse_file(char *filepath, uint64_t *code_start, uint64_t *code_end);

#endif // __CODE_H__