#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "machine.h"
#include "code.h"

/*
 * Parse a string containing an ARM operand
 */
struct operand_t parse_operand(char *str) {
    struct operand_t operand;
    
    // Parse type
    operand.type = str[0];

    // Parse details
    switch(str[0]) {
    case REGISTER_w:
    case REGISTER_x:
        operand.type = OPERAND_register;
        operand.reg_type = str[0];
        // Special case for zero register
        if (str[1] == 'z') {
            operand.reg_num = 31;
        }
        else {
            operand.reg_num = atoi(str+1);
        }
        break;
    case REGISTER_sp:
    case REGISTER_pc:
        operand.type = OPERAND_register;
        operand.reg_type = str[0];
        break;
    case OPERAND_constant:
        operand.type = OPERAND_constant;
        operand.reg_type = '\0';
        operand.constant = (int)strtol(str+1, NULL, 0);
        break;
    case OPERAND_memory:
        // Parse type of base register
        operand.reg_type = str[1];

        // Parse base register
        int i = 2;
        while (str[i] != ',' && str[i] != ']') {
            i++;
        }
        int has_offset = (str[i] == ',');
        str[i] = '\0';
        if (operand.reg_type == REGISTER_w || operand.reg_type == REGISTER_x) {
            // Special case for zero register
            if (str[2] == 'z') {
                operand.reg_num = 31;
            }
            else {
                operand.reg_num = atoi(str+2);
            }
        }

        // Parse offset
        if (has_offset) {
            while (str[i] != '#') {
                i++;
            }
            str[strlen(str)-1] = '\0';
            operand.constant = (int)strtol(str+i+1, NULL, 0);
        }
        else {
            operand.constant = 0;
        }
        break;
    // Operand is a memory address in a branch instruction
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        operand.type = OPERAND_address;
        operand.reg_type = '\0';
        operand.constant = (int)strtol(str, NULL, 16);
        break;
    case '<':
        operand.type = OPERAND_NULL;
        operand.reg_type = '\0';
        break;
    default:
        fprintf(stderr, "! Cannot parse operand: %s\n", str);
        operand.type = OPERAND_NULL;
        operand.reg_type = '\0';
    }

    return operand;
}

/*
 * Print a register operand in human-readable form
 */
void print_register_operand(struct operand_t operand) {
    switch(operand.reg_type) {
    case REGISTER_w:
    case REGISTER_x:
        // Special case w/xzr
        if (operand.reg_num == 31) {
            printf("%czr", operand.reg_type);
        }
        else {
            printf("%c%d", operand.reg_type, operand.reg_num);
        }
        break;
    case REGISTER_sp:
        printf("sp");
        break;
    case REGISTER_pc:
        printf("pc");
        break;
    }
}

/*
 * Print an operand in a human-readable form
 */
void print_operand(struct operand_t operand) {
    switch(operand.type) {
    case OPERAND_register:
        print_register_operand(operand);
        break;
    case OPERAND_constant:
        printf("#%d", operand.constant);
        break;
    case OPERAND_memory: 
        printf("[");
        print_register_operand(operand);
        if (operand.constant != 0) {
            printf(", #%d", operand.constant);
        }
        printf("]");
        break;
    case OPERAND_address:
        printf("%x", operand.constant);
        break;
    case OPERAND_NULL:
        break;
    default:
        fprintf(stderr, "! Cannot print operand wih type %c\n", operand.type);
    }
}

/*
 * Parse a string containing an ARM assembly instruction.
 */
struct instruction_t parse_instruction(char *str) {
    struct instruction_t instruction;

    // Parse operation
    int i = 0;
    while (str[i] != '\0' && !isspace(str[i])) {
        i++;
    }
    str[i++] = '\0';
    strncpy((char *)&instruction.operation, str, 4);

    int num_operands = 0;

    // Locate and store the operands; the delimeter between operands is a whitespace character
    int sqbkt = 0;
    int j = i;
    int comment = 0;
    while (str[i] != '\0' && num_operands < MAX_OPERANDS && comment < 2) {
        if ('[' == str[i]) {
            sqbkt = 1;
        }
        else if (']' == str[i]) {
            sqbkt = 0;
        }
        else if ('/' == str[i]) {
            comment++;
        }
        else if (isspace(str[i]) && !sqbkt) {
            str[i] = '\0';
            if (str[i-1] == ',') {
                str[i-1] = '\0';
            }
            if (i != j) {
                instruction.operands[num_operands] = parse_operand(str+j);
                num_operands++;
            }
            j = i+1;
        }
        i++;
    }

    while (num_operands < MAX_OPERANDS) {
        instruction.operands[num_operands].type = OPERAND_NULL;
        num_operands++;
    }

    //print_instruction(instruction);
    return instruction;
}

/*
 * Print an instruction in a human-readable form
 */
void print_instruction(struct instruction_t instruction) {
    // Print operation
    char name[5];
    strncpy(name, (char *)&instruction.operation, 4); // Operation code is based on name
    name[4] = '\0';
    printf("%s ", name);

    // Print operands
    print_operand(instruction.operands[0]);
    if (instruction.operands[1].type != OPERAND_NULL) {
        printf(", ");
        print_operand(instruction.operands[1]);
    }
    if (instruction.operands[2].type != OPERAND_NULL) {
        printf(", ");
        print_operand(instruction.operands[2]);
    }
    printf("\n");
}

/*
 * Parse a file containing the output from objdump; return an array of instructions
 */
struct instruction_t *parse_file(char *filepath, uint64_t *code_start, uint64_t *code_end) {
    // Open source code
    FILE *source = fopen(filepath, "r");
    if (NULL == source) {
        perror("Failed to load code");
        exit(1);
    }

    // Read in lines of source code
    char line[100];
    int line_num = 0;
    int instruction_offset = 0;
    struct instruction_t *instructions = malloc(sizeof(struct instruction_t));
    while(fgets(line, sizeof(line), source) != NULL) {
        line_num++;
        // Skip blank lines
        if (line[0] == '\n' || line[strlen(line)-2] == ':') {
            continue;
        }

        //printf("%s", line);

        // Skip indendentation
        int i = 0;
        while (isspace(line[i])) {
            i++;
        }

        // Extract address 
        char *addr = line+i;
        while (!isspace(line[i])) {
            i++;
        }
        line[i] = '\0';
        line[i++] = '\0';

        // Store address of first instruction
        char *endptr = NULL;
        if (*code_start == 0) {
            *code_start = strtol(addr, &endptr, 16);
            // If we didn't parse a hex number, skip the line
            if (endptr == addr && *code_start == 0) {
                continue;
            }
        }

        uint64_t previous_code_end = *code_end;
        *code_end = strtol(addr, &endptr, 16);
        if (endptr == addr && *code_start == 0) {
            continue;
        }
        assert(previous_code_end < *code_end);

        // Ignore instruction encoding
        while (!isspace(line[i])) {
            i++;
        }
        while(isspace(line[i])) {
            i++;
        }

        // Parse instruction
        instruction_offset = (*code_end - *code_start) / INSTRUCTION_SIZE;
        instructions = realloc(instructions,
                               sizeof(struct instruction_t) *
                                   (instruction_offset + 1));
        instructions[instruction_offset] = parse_instruction(line+i);
    }

    // Last instruction has no operation 
    instruction_offset++;
    instructions = realloc(instructions,
                           sizeof(struct instruction_t) *
                               (instruction_offset + 1));
    instructions[instruction_offset].operation = OPERATION_NULL;
    fclose(source);

    /*
    // Debugging printf()s.
    fprintf(stderr, "Code goes from %lx to %lx, %ld instructions\n",
            *code_start,
            *code_end,
            (*code_end - *code_start) / INSTRUCTION_SIZE);
    int k = 0;
    while(machine.code[k] != NULL) {
        printf("%p %s\n", (void *)(machine.code_start + k * 4), machine.code[k]);
        k++;
    }*/
    return instructions;
}