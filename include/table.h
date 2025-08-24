#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    COLUMN_INT,
    COLUMN_TEXT
} ColumnType;

typedef struct {
    char name[32];
    ColumnType type;
} Column;

#define MAX_COLUMNS 32

typedef struct {
    uint32_t num_columns;
    Column columns[MAX_COLUMNS];
} TableSchema;

size_t compute_row_size(const TableSchema* schema);

typedef struct {
    uint8_t* data;
} NewRow;


NewRow* create_row(const TableSchema* schema);

size_t get_column_offset(const TableSchema* schema, int col_index);

void set_int_value(const TableSchema* schema, NewRow* row, int col_index, int32_t value);
void set_text_value(const TableSchema* schema, NewRow* row, int col_index, const char* text);


#define TABLE_MAX_PAGES 100

typedef struct {
    char name[32];
    TableSchema schema;
    void* pages[TABLE_MAX_PAGES];
    uint32_t num_rows;
} Table;

void free_table(Table* table);
Table* new_table();

#endif
