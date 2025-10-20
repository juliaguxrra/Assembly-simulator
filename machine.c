#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "machine.h"
#include "code.h"

struct machine_t machine;

/*
 * Allocate more space to keep track of values on the simulated stack.
 */
void grow_stack(uint64_t new_sp) {
    // Grow the stack upwards
    if (new_sp < machine.stack_top) {
        // Round down to a multiple of word size
        if (new_sp % WORD_SIZE_BYTES != 0) {
            new_sp -= new_sp % WORD_SIZE_BYTES;
        }

        // Allocate space and copy over old values 
        void *new_stack = malloc(machine.stack_bot - new_sp + 1);
        memset(new_stack, 0, machine.stack_top - new_sp);
        if (machine.stack != NULL) {
            memcpy(new_stack + (machine.stack_top - new_sp), machine.stack, machine.stack_bot - machine.stack_top + 1);
            free(machine.stack);
        }

        // Update machine
        machine.stack = new_stack;
        machine.stack_top = new_sp;
    }
    // Grow the stack downwards
    else if (new_sp > machine.stack_bot) {
        // Round up to a multiple of word size
        if (new_sp % WORD_SIZE_BYTES != 0) {
            new_sp += WORD_SIZE_BYTES - (new_sp % WORD_SIZE_BYTES);
        }
        else {
            new_sp += WORD_SIZE_BYTES;
        }

        // Allocate space and copy over old values 
        void *new_stack = malloc(new_sp - machine.stack_top);
        memset(new_stack + (machine.stack_bot - machine.stack_top), 0, new_sp - machine.stack_bot);
        if (machine.stack != NULL) {
            memcpy(new_stack, machine.stack, machine.stack_bot - machine.stack_top);
            free(machine.stack);
        }

        // Update machine
        machine.stack = new_stack;
        machine.stack_bot = new_sp - 1;
    }
}

/*
 * Initialize the machine
 */
void init_machine(uint64_t sp, uint64_t pc, char *code_filepath) {
    // Populate general purpose registers
    for (int i = 0; i <= 30; i++) {
        machine.registers[i] = REGISTER_NULL;
    }
    
    // Populate special purpose registers
    machine.sp = sp;
    machine.pc = pc;
    
    // Load code
    machine.code = parse_file(code_filepath, &(machine.code_top), &(machine.code_bot));

    // Prepare stack
    machine.stack_top = sp;
    machine.stack_bot = sp + WORD_SIZE_BYTES - 1;
    machine.stack = malloc(WORD_SIZE_BYTES);
    memset(machine.stack, 0, WORD_SIZE_BYTES);

    // Clear all condition codes
    machine.conditions = 0;
}

void print_memory() {
    // Print condition codes
    printf("Condition codes:");
    if (machine.conditions & CONDITION_ZERO) {
        printf(" Z");
    }
    if (machine.conditions & CONDITION_NEGATIVE) {
        printf(" N");
    }
    if (machine.conditions & CONDITION_POSITIVE) {
        printf(" P");
    }
    printf("\n");

    // Print the value of all used registers
    printf("Registers:\n");
    for (int i = 0; i <= 30; i++) {
        if (machine.registers[i] != REGISTER_NULL) {
            printf("\tw/x%d = 0x%lx\n", i, machine.registers[i]);
        }
    }
    printf("\tsp = 0x%lX\n", machine.sp);
    printf("\tpc = 0x%lX\n", machine.pc);

    // If necessary, grow the stack before printing it
    if (machine.sp < machine.stack_top || machine.sp > machine.stack_bot) {
        grow_stack(machine.sp);
    }

    // Print the value of all words on the stack
    printf("Stack:\n");
    unsigned char *stack = machine.stack;
    for (int i = 0; i < (machine.stack_bot - machine.stack_top); i += 8) {
        printf("\t");

        if (machine.sp == i + machine.stack_top) {
            printf("%10s ", "sp->");
        }
        else {
            printf("           ");
        }

        printf("+-------------------------+\n");
        printf("\t0x%08lX | ", i + machine.stack_top);
        for (int j = 0; j < 8; j++) {
            printf("%02X ", stack[i+j]);
        }
        printf("|\n");
    }
    printf("\t           +-------------------------+\n");
}

/*
 * Get the next instruction to execute
 */
struct instruction_t fetch() {
    int index = (machine.pc - machine.code_top) / 4;
    return machine.code[index];
}

/*
 * Get the value associated with a constant or register operand.
 */
uint64_t get_value(struct operand_t operand) {
    assert(operand.type == OPERAND_constant || operand.type == OPERAND_address || operand.type == OPERAND_register);
    switch(operand.type){ // Check what type of operand it is
        case OPERAND_constant:
        case OPERAND_address:
            return operand.constant;
        case OPERAND_register:
            switch(operand.reg_type){ // If the operand is a register then check what type of register it is
                case REGISTER_x:
                    return machine.registers[operand.reg_num];
                case REGISTER_w:
                    return (uint32_t)machine.registers[operand.reg_num]; // Zero-extend 32-bit to 64-bit
                case REGISTER_sp:
                    return machine.sp;
                case REGISTER_pc:
                    return machine.pc;
                default:
                    printf("Invalid register type\n");
            }
        default:
            printf("Invalid operand type\n");
    }
    return 0;
}

/*
 * Put a value in a register specified by an operand.
 */
void put_value(struct operand_t operand, uint64_t value) {
    assert(operand.type == OPERAND_register);
    switch(operand.reg_type){ // Check what type of register it is
        case REGISTER_w:
            machine.registers[operand.reg_num] = (uint32_t)value; // Store only 32-bits for a w register, zero the upper bits
            break;
        case REGISTER_x:
            machine.registers[operand.reg_num] = value;
            break;
        case REGISTER_sp:
            machine.sp = value;
            break;
        case REGISTER_pc:
            machine.pc = value;
            break;
        default:
            printf("Invalid operand reg_type\n");
    }
}

/*
 * Get the memory address associated with a memory operand.
 */
uint64_t get_memory_address(struct operand_t operand) {
    assert(operand.type == OPERAND_memory);
    // Create a register operand to extract the base register value
    struct operand_t reg_operand;
    reg_operand.type = OPERAND_register; // Set it to an operand type of register
    reg_operand.reg_type = operand.reg_type;
    reg_operand.reg_num = operand.reg_num;

    uint64_t base_value = get_value(reg_operand); // Get its value
    return base_value + operand.constant; // And return its value with the offset
}

// Handles all arithmetic operations
void execute_arithmetic(struct instruction_t instruction) {
    uint64_t op1 = get_value(instruction.operands[1]);
    uint64_t op2 = get_value(instruction.operands[2]);
    uint64_t result;
    switch(instruction.operation) {
        case OPERATION_add:
            result = op1 + op2;
            break;
        case OPERATION_sub:
        case OPERATION_subs:
            result = op1 - op2;
            break;
        case OPERATION_mul:
            result = op1 * op2;
            break;
        case OPERATION_sdiv:
        case OPERATION_udiv:
            result = op1 / op2;
            break;
    }
    put_value(instruction.operands[0], result);
}

// Handles all bitwise operations
void execute_bitwise(struct instruction_t instruction){
    uint64_t op1 = get_value(instruction.operands[1]);
    uint64_t op2 = get_value(instruction.operands[2]);
    uint64_t result;
    switch(instruction.operation){ // Check which bitwise operation was called
        case OPERATION_neg:
            result = -op1;
            break;
        case OPERATION_lsl: // Executes left shift
            result = op1 << op2;
            break;
        case OPERATION_lsr: // Executes right shift
            result = op1 >> op2;
            break;
        case OPERATION_and: // Executes and
            result = op1 & op2;
            break;
        case OPERATION_orr: // Executes or
            result = op1 | op2;
            break;
        case OPERATION_eor: // Executes exclusive or
            result = op1 ^ op2;
            break;
    }
    put_value(instruction.operands[0], result);
}

void execute_ldr(struct instruction_t instruction){
    uint64_t simulated_address = get_memory_address(instruction.operands[1]); // Computes simulated stack address
    uint64_t offset = simulated_address - machine.stack_top; // Then finds the offset between that address and the simulated stack top
    uint8_t* real_address = machine.stack + offset; // Then computes real memory address
    switch(instruction.operands[0].reg_type){ // Checks which register type the destination register is
        case REGISTER_w:
            put_value(instruction.operands[0], *(uint32_t*)real_address); // Loads the 32-bit memory address
            break;
        case REGISTER_x:
            put_value(instruction.operands[0], *(uint64_t*)real_address); // Loads the 64-bit memory address
            break;
        case REGISTER_sp:
            machine.sp = *(uint64_t*)real_address;
            break;
    }
}

void execute_str(struct instruction_t instruction){
    uint64_t simulated_address = get_memory_address(instruction.operands[1]); // Computes the simulated stack address
    uint64_t offset = simulated_address - machine.stack_top; // Then finds the offset
    uint8_t* real_address = machine.stack + offset; // Then computes the real memory address
    switch(instruction.operands[0].reg_type){
        case REGISTER_w:
            *(uint32_t*)real_address = (uint32_t)get_value(instruction.operands[0]);
            break;
        case REGISTER_x:
            *(uint64_t*)real_address = (uint64_t)get_value(instruction.operands[0]);
            break;
    }
}

void execute_cmp(struct instruction_t instruction){
    uint64_t operand1= get_value(instruction.operands[0]);//register with first value
    uint64_t operand2= get_value(instruction.operands[1]);//constant value or register
    if(operand1==operand2){
        machine.conditions= CONDITION_ZERO; // Sets the condition to zero if the operands are the same
    } 
    else if(operand1< operand2){
        machine.conditions= CONDITION_NEGATIVE; // Sets the condition to negative if operand1 < operand2
    } 
    else{
        machine.conditions= CONDITION_POSITIVE; // Sets the condition to positive if operand1 > operand2
    }
}

void execute_branches(struct instruction_t instruction){
        uint64_t target = get_value(instruction.operands[0]);
            switch(instruction.operation) {
        case OPERATION_b:
            machine.pc = target; // Set pc to the target address
            break;
        case OPERATION_bl:
            machine.registers[30] = machine.pc + 0x4; // Update x30
            machine.pc = target;
            break;
        case OPERATION_beq:
            if (machine.conditions & CONDITION_ZERO) // Checks if they are equal
                machine.pc = target;
            break;
        case OPERATION_bne:
            if (!(machine.conditions & CONDITION_ZERO)) // Checks if they are not equal
                machine.pc = target;
            break;
        case OPERATION_blt:
            if (machine.conditions & CONDITION_NEGATIVE) // Checks if operand1 is less than operand2
                machine.pc = target;
            break;
        case OPERATION_bgt:
            if (machine.conditions & CONDITION_POSITIVE) // Checks if operand1 is greater than operand2
                machine.pc = target;
            break;
        case OPERATION_ble:
            if ((machine.conditions & CONDITION_ZERO) || (machine.conditions & CONDITION_NEGATIVE)) // Checks if they are equal or operand1 is less than operand2
                machine.pc = target;
            break;
        case OPERATION_bge:
            if ((machine.conditions & CONDITION_ZERO) || (machine.conditions & CONDITION_POSITIVE)) // Checks if they are equal or operand1 is greater than operand2
                machine.pc = target;
            break;
        default:
            printf("Invalid branch operation\n");
    }


}

void execute_clz(struct instruction_t instruction) {
    uint64_t value = get_value(instruction.operands[1]); // Get the value from the register
    int count = 0;
    int amount; // Used to get how many bits are needed to check based on the register type
    switch(instruction.operands[0].reg_type){
        case REGISTER_w:
            amount = 32; // Only needs to check 32 bits
            value &= 0xFFFFFFFF; // Mask to 32 bits
            break;
        case REGISTER_x:
            amount = 64; // Has to check all 64 bits
            break;
    }
    for (int i = amount - 1; i >= 0; i--){
        if ((value >> i) & 1) {
            break; // Found a 1
        }
        count++;
    }
    put_value(instruction.operands[0], count); // Sets the register destination to whatever the count was
}

void execute_ldrb(struct instruction_t instruction){ // Loads register byte
    uint64_t simulated_address = get_memory_address(instruction.operands[1]); 
    uint64_t offset = simulated_address - machine.stack_top; 
    uint8_t* real_address = machine.stack + offset; // Casts the real address to 1 byte
    put_value(instruction.operands[0], *real_address);
}

void execute_strb(struct instruction_t instruction){//stores register byte
    uint64_t simulated_address = get_memory_address(instruction.operands[1]);
    uint64_t offset = simulated_address-machine.stack_top;
    uint8_t *real_address = machine.stack+ offset;
    *real_address = (uint8_t)(get_value(instruction.operands[0])); // Casts the value to only be 1 byte
}

void execute_ret(){
    machine.pc = machine.registers[30]; // Return pc back to what x30 is
}

/*
 * Execute an instruction
 */
void execute(struct instruction_t instruction) {
    switch(instruction.operation) {
        case OPERATION_add:
        case OPERATION_sub:
        case OPERATION_subs:
        case OPERATION_mul:
        case OPERATION_sdiv:
        case OPERATION_udiv:
            execute_arithmetic(instruction);
            break;
        case OPERATION_neg:
        case OPERATION_lsl:
        case OPERATION_lsr:
        case OPERATION_and:
        case OPERATION_orr:
        case OPERATION_eor:
            execute_bitwise(instruction);
            break;
        case OPERATION_mov:
            put_value(instruction.operands[0], get_value(instruction.operands[1]));
            break;
        case OPERATION_ldr:
            execute_ldr(instruction);
            break;
        case OPERATION_str:
            execute_str(instruction);
            break;
        case OPERATION_b:
        case OPERATION_bl:
        case OPERATION_bne:
        case OPERATION_beq:
        case OPERATION_blt:
        case OPERATION_bgt:
        case OPERATION_ble:
        case OPERATION_bge:
            execute_branches(instruction);
            break;
        case OPERATION_cmp:
            execute_cmp(instruction);
            break;
        case OPERATION_nop:
            break;
        case OPERATION_clz:
            execute_clz(instruction);
            break;
        case OPERATION_ldrb:
            execute_ldrb(instruction);
            break;
        case OPERATION_strb:
            execute_strb(instruction);
            break;
        case OPERATION_ret:
            execute_ret();
            break;
        default:
            printf("!!Instruction not implemented!!\n");
    }
}