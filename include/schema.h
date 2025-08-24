#ifndef SCHEMA_H
#define SCHEMA_H

#include <stddef.h>
#include <stdint.h>

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
    char name[32];
    uint32_t num_columns;
    Column columns[MAX_COLUMNS];
} TableSchema;

size_t compute_row_size(const TableSchema* schema);

#endif
