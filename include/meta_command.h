#ifndef META_COMMAND_H
#define META_COMMAND_H

#include "input_buffer.h"

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED
} MetaCommandResult;
MetaCommandResult do_meta_command(InputBuffer* input_buffer);



#endif