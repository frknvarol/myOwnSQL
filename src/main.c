#include <stdio.h>
#include <stdlib.h>
#include "input_buffer.h"
#include "meta_command.h"
#include "statement.h"
#include "schema.h"


void print_prompt() { printf("> "); }

int main(void) {
    InputBuffer* input_buffer = new_input_buffer();
    while (1) {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer->buffer[0] == '.') {
            switch (do_meta_command(input_buffer)) {
                case META_COMMAND_SUCCESS:
                    break;
                case META_COMMAND_UNRECOGNIZED:
                    printf("Unrecognized meta-command '%s'\n", input_buffer->buffer);
                case META_COMMAND_EXIT:
                    close_input_buffer(input_buffer);
                    exit(EXIT_SUCCESS);
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
            case (PREPARE_TABLE_NOT_FOUND_ERROR):
                printf("Table not found.\n");
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
        free_statement(&statement);
    }
}