#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "meta_command.h"
#include "input_buffer.h"


MetaCommandResult do_meta_command(const InputBuffer* input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        return META_COMMAND_EXIT;

    }
    if (strcmp(input_buffer->buffer, ".help") == 0) {
        printf("\nMeta-commands:\n");
        printf("  .help      Show this help\n");
        printf("  .exit      Exit the program\n\n");
        return META_COMMAND_SUCCESS;
    }

    return META_COMMAND_UNRECOGNIZED;
}





