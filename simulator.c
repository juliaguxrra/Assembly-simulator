#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "machine.h"
#include "code.h"

int main(int argc, char **argv) {
    // Check for valid command line arguments
    if (argc != 4) {
        printf("Usage: %s CODE_FILEPATH PC SP\n", argv[0]);
        exit(1);
    }

    // Get command line arguments
    char *code_filepath = argv[1];
    uint64_t pc = strtol(argv[2], NULL, 0);
    uint64_t sp = strtol(argv[3], NULL, 0);

    // Initialize machine
    init_machine(sp, pc, code_filepath);

    // Fetch and execute instructions
    print_memory();
    printf("\n\n");
    while (machine.pc <= machine.code_bot) {
        struct instruction_t instruction = fetch();
        print_instruction(instruction);
        uint64_t pc_before = machine.pc;
        execute(instruction);
        if (machine.pc == pc_before) {
            machine.pc += 4;
        }
        print_memory();
        printf("\n\n");
    }

    // Clean-up
    free(machine.stack);
    free(machine.code);
}