#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "statement.h"
#include "input_buffer.h"
#include "table.h"
#include "database.h"

Database global_db;

PrepareResult prepare_statement(const InputBuffer* input_buffer, Statement* statement) {
    if (strncmp(input_buffer->buffer, "insert into", 11) == 0) {
        statement->type = STATEMENT_INSERT;

        char* start = input_buffer->buffer + 11;
        sscanf(start, "%31s", statement->table_name);
        char* open_paren = strchr(input_buffer->buffer, '(');
        char* close_paren = strchr(input_buffer->buffer, ')');
        if (!open_paren || !close_paren) return PREPARE_SYNTAX_ERROR;

        char column_definitions[256];
        strncpy(column_definitions, open_paren + 1, close_paren - open_paren - 1);
        column_definitions[close_paren - open_paren - 1] = '\0';

        Table* table = find_table(&global_db, statement->table_name);

        char* token = strtok(column_definitions, ",");

        u_int32_t col_num = 0;

        while (token != NULL && col_num < statement->num_columns) {}

        int args_assigned = sscanf(
            input_buffer->buffer, "insert %d %31s %254s",
            &statement->row.id,
            statement->row.username,
            statement->row.email
        );
        if (args_assigned < 3) {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }

    if (strncmp(input_buffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
        char* start = input_buffer->buffer + 6;
        sscanf(start, "%31s", statement->table_name);
        return PREPARE_SUCCESS;
    }


    if (strncmp(input_buffer->buffer, "create table", 12) == 0) {

        char* start = input_buffer->buffer + 12;

        sscanf(start, "%31s", statement->table_name);

        char* open_paren = strchr(input_buffer->buffer, '(');
        char* close_paren = strchr(input_buffer->buffer, ')');
        if (!open_paren || !close_paren) return PREPARE_SYNTAX_ERROR;

        char column_definitions[256];
        strncpy(column_definitions, open_paren + 1, close_paren - open_paren - 1);
        column_definitions[close_paren - open_paren - 1] = '\0';

        char* token = strtok(column_definitions, ",");
        statement->num_columns = 0;

        while (token != NULL && statement->num_columns < MAX_COLUMNS) {
            char col_name[32], col_type[16];
            sscanf(token, "%31s %15s", col_name, col_type);

            strcpy(statement->columns[statement->num_columns].name, col_name);
            if (strcasecmp(col_type, "INT") == 0) {
                statement->columns[statement->num_columns].type = COLUMN_INT;
            }
            else if (strcasecmp(col_type, "TEXT") == 0) {
                statement->columns[statement->num_columns].type = COLUMN_TEXT;
            }

            else {
                return PREPARE_SYNTAX_ERROR;
            }

            statement->num_columns++;
            token = strtok(NULL, ",");
        }

        statement->type = STATEMENT_CREATE_TABLE;

        return PREPARE_SUCCESS;

    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

void serialize_row(Row* source, void* destination) {
    memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy((char*)destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    memcpy((char*)destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), (char*)source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), (char*)source + EMAIL_OFFSET, EMAIL_SIZE);
}

void* row_slot(Table* table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    if (page == NULL) {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return (char*)page + byte_offset;
}

ExecuteResult execute_statement(Statement* statement) {
    switch (statement->type) {
        case STATEMENT_INSERT:
            return execute_insert(statement);
        case STATEMENT_SELECT:
            return execute_select(statement);
        case STATEMENT_CREATE_TABLE:
            return execute_create_table(statement);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(Statement* statement) {
    Table* table = find_table(&global_db, statement->table_name);

    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_FAIL;
    }
    Row* row_to_insert = &(statement->row);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement) {
    Table* table = find_table(&global_db, statement->table_name);
    Row row;
    for (uint32_t i = 0; i < table->num_rows; i++) {
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_create_table(Statement* statement) {
    Table* new_table = malloc(sizeof(Table));
    if (!new_table) {
        printf("Error: memory allocation failed.\n");
        return EXECUTE_FAIL;
    }

    strncpy(new_table->schema.name, statement->table_name, sizeof(new_table->schema.name));
    new_table->schema.num_columns = statement->num_columns;
    for (int i = 0; i < statement->num_columns; i++) {
        new_table->schema.columns[i] = statement->columns[i];
    }

    new_table->num_rows = 0;

    if (add_table(&global_db, new_table) != 0) {
        free(new_table);
        return EXECUTE_FAIL;
    }

    printf("Table '%s' created with %d columns.\n",
           new_table->schema.name, new_table->schema.num_columns);
    return EXECUTE_SUCCESS;
}


void print_row(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}




