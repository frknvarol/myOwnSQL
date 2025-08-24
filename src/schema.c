#include "schema.h"
#include <stddef.h>

size_t compute_row_size(const TableSchema* schema) {
    size_t size = 0;
    for (int i = 0; i < schema->num_columns; i++) {
        switch (schema->columns[i].type) {
            case COLUMN_INT:  size += sizeof(int); break;
            case COLUMN_TEXT: size += 255; break;
        }
    }
    return size;
}
