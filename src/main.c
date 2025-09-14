#include <stdio.h>
#include <stdbool.h>

#include "input_buffer.h"
#include "meta_command.h"
#include "statement.h"
#include "table.h"
#include "schema.h"


void print_prompt() { printf("db > "); }

int main(int argc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();
    while (true) {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer->buffer[0] == '.') {
            if (do_meta_command(input_buffer) == META_COMMAND_UNRECOGNIZED) {
                printf("Unrecognized meta-command '%s'\n", input_buffer->buffer);
            }
            continue;
        }

        Statement statement;
        switch (prepare_statement(input_buffer, &statement)) {
            case (PREPARE_SUCCESS):
                break;
            case (PREPARE_SYNTAX_ERROR):
                printf("Syntax error. Could not parse statement.\n");
                continue;
            case (PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
                continue;
            case (PREPARE_INSERT_TYPE_ERROR):
                printf("Insert type error.\n");
                continue;
            case (PREPARE_INSERT_VARCHAR_SIZE_ERROR):
                printf("The size of the VARCHAR given is larger than the determined size.\n");
                continue;
            }



        switch (execute_statement(&statement)) {
            case (EXECUTE_SUCCESS):
                printf("Executed.\n");
                break;
            case (EXECUTE_FAIL):
                printf("Error.\n");
                break;
            }
    }
}