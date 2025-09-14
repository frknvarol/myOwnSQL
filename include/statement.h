#ifndef STATEMENT_H
#define STATEMENT_H

#include <stdint.h>
#include "input_buffer.h"
#include "table.h"


typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_CREATE_TABLE
}StatementType;


#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100

#define ROW_SIZE sizeof(Row)
#define ROWS_PER_PAGE (PAGE_SIZE / ROW_SIZE)
#define TABLE_MAX_ROWS (ROWS_PER_PAGE * TABLE_MAX_PAGES)


#define TABLE_MAX_PAGES 100

typedef struct {
    StatementType type;
    char table_name[32];
    uint32_t num_columns;
    Row row;
    Column columns[MAX_COLUMNS];
} Statement;

void serialize_row(const TableSchema* schema, Row* source, void* destination);
void deserialize_row(const TableSchema* schema, void* source, Row* destination);


typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_INSERT_TYPE_ERROR,
    PREPARE_INSERT_VARCHAR_SIZE_ERROR
} PrepareResult;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_FAIL
}ExecuteResult;

PrepareResult prepare_statement(const InputBuffer* input_buffer, Statement* statement);
ExecuteResult execute_statement(Statement* statement);
ExecuteResult execute_insert(Statement* statement);
ExecuteResult execute_select(Statement* statement);
ExecuteResult execute_create_table(const Statement* statement);
void print_row(const TableSchema* schema, const Row* row);
char* find_close_parenthesis(char* open_parenthesis);




#endif