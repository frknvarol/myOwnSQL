#ifndef TABLE_H
#define TABLE_H

#include "schema.h"
#include <stdint.h>

#define TABLE_MAX_PAGES 100

typedef struct {
    char name[32];
    TableSchema schema;
    void* pages[TABLE_MAX_PAGES];
    uint32_t num_rows;
    size_t row_size;
} Table;

void free_table(Table* table);
Table* new_table();

#endif
