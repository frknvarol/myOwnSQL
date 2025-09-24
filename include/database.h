#ifndef DATABASE_H
#define DATABASE_H

#include <stdint.h>
#include "table.h"

#define MAX_TABLES 32

typedef struct {
    char name[32];
    uint32_t num_tables;
    Table* tables[MAX_TABLES];
} Database;

/*
typedef struct {
    Database* db;
} Session;
*/

extern Database global_db;

void init_database(Database* db, const char* name);
Table* find_table(Database* db, const char* table_name);
int add_table(Database* db, Table* table);

#endif
