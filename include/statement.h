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
    char* column_name;
    uint32_t column_index;
    char* value;
} Condition;

typedef struct {
    char table_name[32];
    uint32_t num_columns;
    Column columns[MAX_COLUMNS];
    uint32_t primary_col_index;
} CreateStatement;

typedef struct {
    char table_name[32];
    Row row;
} InsertStatement;

typedef struct {
    char table_name[32];
    uint32_t selected_col_indexes[MAX_COLUMNS];
    uint32_t selected_col_count;
    int has_condition;
    Condition conditions[MAX_COLUMNS];
    uint32_t condition_count;
} SelectStatement;

typedef struct {
    StatementType type;
    union {
        CreateStatement create_stmt;
        InsertStatement insert_stmt;
        SelectStatement select_stmt;
    };
} Statement;


void serialize_row(const TableSchema* schema, const Row* source, void* destination);
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
ExecuteResult execute_insert(InsertStatement* insert_statement);
ExecuteResult execute_select(const SelectStatement* select_statement);
ExecuteResult execute_create_table(const CreateStatement* create_statement);
void print_row(const TableSchema* schema, const Row* row, const SelectStatement* select_statement);
char* find_close_parenthesis(char* open_parenthesis);




#endif