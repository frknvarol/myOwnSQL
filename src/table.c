#include "table.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

size_t compute_row_size(const TableSchema* schema) {
    size_t size = 1; // first byte is for deleted flag
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
    size_t offset = 1;
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
        if (table->pages[i] != NULL) {
            free(table->pages[i]);
            table->pages[i] = NULL;
        }
    }

    if (table->tree != NULL) {
        free_tree(table->tree);
        table->tree = NULL;
    }
    free(table);
}

// Table row allocator function
void* row_slot(Table* table, uint32_t row_num) {
    size_t row_size = compute_row_size(&table->schema);
    size_t rows_per_page = PAGE_SIZE / row_size;

    size_t page_num = row_num / rows_per_page;
    if (table->pages[page_num] == NULL) {
        table->pages[page_num] = malloc(PAGE_SIZE);
        if (!table->pages[page_num]) {
            perror("malloc failed");
            exit(1);
        }
    }
    void* page = table->pages[page_num];


    size_t row_offset = row_num % rows_per_page;
    size_t byte_offset = row_offset * row_size;
    return (char*)page + byte_offset;
}

void serialize_row(const TableSchema* schema, const Row* source, void* destination) {

    *(uint8_t*)destination = 0;

    size_t offset = 1;
    for (int i = 0; i < schema->num_columns; i++) {
        size_t col_size = (schema->columns[i].type == COLUMN_INT) ? sizeof(int32_t) : 256;
        memcpy((char*)destination + offset, source->data + offset, col_size);
        offset += col_size;
    }
}

void deserialize_row(const TableSchema* schema, void* source, Row* destination) {
    size_t offset = 0;
    for (int i = 0; i < schema->num_columns; i++) {
        size_t col_size = (schema->columns[i].type == COLUMN_INT) ? sizeof(int32_t) : 256;
        memcpy(destination->data + offset, (char*)source + offset, col_size);
        offset += col_size;
    }
}

/*
void print_row(const TableSchema* schema, const Row* row, const SelectStatement* select_statement) {

    printf("(");
    if (select_statement->selected_col_count == 0) {
        for (int i = 0; i < schema->num_columns; i++) {
            if (i > 0) printf(", ");
            if (schema->columns[i].type == COLUMN_INT) {
                int32_t val;
                memcpy(&val, row->data + get_column_offset(schema, i), sizeof(int32_t));
                printf("%d", val);
            } else if (schema->columns[i].type == COLUMN_VARCHAR){
                char buf[257];
                memcpy(buf, row->data + get_column_offset(schema, i), 256);
                buf[256] = '\0';
                printf("%s", buf);
            }
        }
    } else {
        for (uint32_t i = 0; i < select_statement->selected_col_count; i++) {
            if (i > 0) printf(", ");
            uint32_t index = select_statement->selected_col_indexes[i];
            if (schema->columns[index].type == COLUMN_INT) {
                int32_t val;
                memcpy(&val, row->data + get_column_offset(schema, index), sizeof(int32_t));
                printf("%d", val);
            } else if (schema->columns[index].type == COLUMN_VARCHAR){
                char buf[257];
                memcpy(buf, row->data + get_column_offset(schema, index), 256);
                buf[256] = '\0';
                printf("%s", buf);
            }
        }
    }
    printf(")\n");

}
*/
void delete_row(void* row) { // TODO BUG: doesn't work for some reason
    *(uint8_t*)row = 1; // mark as deleted
}