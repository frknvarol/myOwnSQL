#ifndef META_COMMAND_H
#define META_COMMAND_H

#include "input_buffer.h"

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED,
    META_COMMAND_EXIT
} MetaCommandResult;
MetaCommandResult do_meta_command(const InputBuffer* input_buffer);



#endif