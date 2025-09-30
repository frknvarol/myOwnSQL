#ifndef STATEMENT_H
#define STATEMENT_H

#include <stdint.h>
#include "input_buffer.h"
#include "table.h"
#include "lexer.h"


typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_CREATE_TABLE,
    STATEMENT_DROP_TABLE,
    STATEMENT_SHOW_TABLES,
    STATEMENT_CREATE_DATABASE,
    STATEMENT_SHOW_DATABASES,
    STATEMENT_DELETE
}StatementType;

typedef struct {
    char* column_name;
    uint32_t column_index;
    char* value;
    TokenType type;
} Condition;

typedef struct {
    char table_name[32];
    uint32_t num_columns;
    Column columns[MAX_COLUMNS];
    uint32_t primary_col_index;
} CreateTableStatement;

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
    char database_name[32];
} CreateDatabaseStatement;

typedef struct {
} ShowTablesStatement;

typedef struct {
    char table_name[32];
} DropTableStatement;

typedef struct {
    char table_name[32];
    int has_condition;
    Condition conditions[MAX_COLUMNS];
    uint32_t condition_count;
} DeleteStatement;


typedef struct {
    StatementType type;
    union {
        CreateTableStatement create_table_stmt;
        InsertStatement insert_stmt;
        SelectStatement select_stmt;
        DropTableStatement drop_table_stmt;
        CreateDatabaseStatement create_database_stmt;
        ShowTablesStatement show_tables_stmt;
        DeleteStatement delete_stmt;
    };
} Statement;




typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_INSERT_TYPE_ERROR,
    PREPARE_INSERT_VARCHAR_SIZE_ERROR,
    PREPARE_TABLE_NOT_FOUND_ERROR
} PrepareResult;

typedef enum {
    EXECUTE_FAIL,
    EXECUTE_SUCCESS
} ExecuteResult;

PrepareResult prepare_statement(const InputBuffer* input_buffer, Statement* statement);
ExecuteResult execute_statement(const Statement* statement);
ExecuteResult execute_insert(const InsertStatement* insert_statement);
ExecuteResult execute_select(const SelectStatement* select_statement);
ExecuteResult execute_create_table(const CreateTableStatement* create_statement);
ExecuteResult execute_drop_table(const DropTableStatement* drop_table_statement);
ExecuteResult execute_show_tables();
ExecuteResult execute_delete(const DeleteStatement* delete_statement);
void print_row(const TableSchema* schema, const Row* row, const SelectStatement* select_statement);
const char* find_close_parenthesis(const char* open_parenthesis);
void free_statement(const Statement* statement);
void free_conditions(uint32_t condition_count, const Condition* conditions);
int filter_rows(const Condition* conditions, uint32_t condition_count, uint32_t row_index, Table* table, Row row);


#endif