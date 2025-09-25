#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include <stddef.h>
#include "binary_plus_tree.h"

typedef enum {
    COLUMN_INT,
    COLUMN_VARCHAR
} ColumnType;

typedef struct {
    char name[32];
    ColumnType type;
    uint32_t size;
    int is_primary;
    uint32_t index;
} Column;

#define MAX_COLUMNS 32

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100

#define ROW_SIZE sizeof(Row)
#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)


#define TABLE_MAX_PAGES 100

typedef struct {
    uint32_t num_columns;
    Column columns[MAX_COLUMNS];
} TableSchema;

size_t compute_row_size(const TableSchema* schema);

typedef struct {
    uint8_t* data;
} Row;


Row* create_row(const TableSchema* schema);

size_t get_column_offset(const TableSchema* schema, int col_index);

void set_int_value(const TableSchema* schema, const Row* row, int col_index, int32_t value);
void set_text_value(const TableSchema* schema, const Row* row, int col_index, const char* text);


#define TABLE_MAX_PAGES 100

typedef struct {
    char name[32];
    TableSchema schema;
    void* pages[TABLE_MAX_PAGES];
    uint32_t num_rows;
    BPTree* tree;
    uint32_t primary_key_index;
} Table;


int extract_primary_key(const TableSchema* schema, const Row* row, int pk_index) ;

void free_table(Table* table);
Table* new_table();
void* row_slot(Table* table, uint32_t row_num);
void delete_row(void* row);
void serialize_row(const TableSchema* schema, const Row* source, void* destination);
void deserialize_row(const TableSchema* schema, void* source, const Row* destination);
int32_t get_column_index(const TableSchema* schema, const char* column_name);

#endif
