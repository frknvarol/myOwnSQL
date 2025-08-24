#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "database.h"

Database global_db;

void init_database(Database* db, const char* name) {
    strncpy(db->name, name, sizeof(db->name));
    db->num_tables = 0;
    for (int i = 0; i < MAX_TABLES; i++) {
        db->tables[i] = NULL;
    }
}

Table* find_table(Database* db, const char* table_name) {
    for (uint32_t i = 0; i < db->num_tables; i++) {
        if (strcmp(db->tables[i]->name, table_name) == 0) {
            return db->tables[i];
        }
    }
    return NULL; // not found
}

int add_table(Database* db, Table* table) {
    if (db->num_tables >= MAX_TABLES) {
        printf("Error: too many tables in database.\n");
        return -1;
    }
    if (find_table(db, table->name) != NULL) {
        printf("Error: table '%s' already exists.\n", table->name);
        return -1;
    }
    db->tables[db->num_tables] = table;
    db->num_tables++;
    return 0;
}
