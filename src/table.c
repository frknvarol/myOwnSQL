#include "table.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

size_t compute_row_size(const TableSchema* schema) {
    size_t size = 0;
    for (int i = 0; i < schema->num_columns; i++) {
        if (schema->columns[i].type == COLUMN_INT) {
            size += sizeof(int32_t);
        } else if (schema->columns[i].type == COLUMN_TEXT) {
            size += 256;
        }
    }
    return size;
}


Row* create_row(const TableSchema* schema) {
    Row* row = malloc(sizeof(Row));
    size_t row_size = compute_row_size(schema);
    row->data = malloc(row_size);
    memset(row->data, 0, row_size);
    return row;
}

size_t get_column_offset(const TableSchema* schema, int col_index) {
    size_t offset = 0;
    for (int i = 0; i < col_index; i++) {
        switch (schema->columns[i].type) {
            case COLUMN_INT:
                offset += sizeof(int32_t);
                break;
            case COLUMN_TEXT:
                offset += 256;
                break;
        }
    }
    return offset;
}



void set_int_value(const TableSchema* schema, Row* row, int col_index, int32_t value) {
    size_t offset = get_column_offset(schema, col_index);
    memcpy(row->data + offset, &value, sizeof(int32_t));
}

void set_text_value(const TableSchema* schema, Row* row, int col_index, const char* text) {
    size_t offset = get_column_offset(schema, col_index);
    strncpy((char*)(row->data + offset), text, 256);
}



Table* new_table() {
    Table* table = (Table*)malloc(sizeof(Table));
    table->num_rows = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = NULL;
    }
    return table;
}

void free_table(Table* table) {
    for (int i = 0; i < TABLE_MAX_PAGES; i++) {
        if (table->pages[i]) {
            free(table->pages[i]);
        }
    }
    free(table);
}
