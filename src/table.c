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
        } else if (schema->columns[i].type == COLUMN_VARCHAR) {
            size += schema->columns[i].size;
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
            case COLUMN_VARCHAR:
                offset += schema->columns[i].size;
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
    const size_t offset = get_column_offset(schema, col_index);
    size_t max_size = 0;

    if (schema->columns[col_index].type == COLUMN_VARCHAR) {
        max_size = schema->columns[col_index].size;
    }

    size_t len = strlen(text);
    if (len > max_size) len = max_size;
    memcpy((char*)(row->data + offset), text, len);
}

int extract_primary_key(const TableSchema* schema, const Row* row, int pk_index) {
    size_t offset = get_column_offset(schema, pk_index);

    int key; //assumes primary key is an int might go into making strings primary as well
    memcpy(&key, row->data + offset, sizeof(int));
    return key;
}



Table* new_table() {
    Table* table = malloc(sizeof(Table));
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
    free_tree(table->tree);
    free(table);
}
