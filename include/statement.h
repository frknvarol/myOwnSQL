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


#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
}Row;

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

extern const uint32_t ID_SIZE;
extern const uint32_t USERNAME_SIZE;
extern const uint32_t EMAIL_SIZE;
extern const uint32_t ID_OFFSET;
extern const uint32_t USERNAME_OFFSET;
extern const uint32_t EMAIL_OFFSET;
extern const uint32_t ROW_SIZE;

extern const uint32_t PAGE_SIZE;
extern const uint32_t ROWS_PER_PAGE;
extern const uint32_t TABLE_MAX_ROWS;

#define TABLE_MAX_PAGES 100

typedef struct {
    StatementType type;
    char table_name[32];
    uint32_t num_columns;
    char column_names[32][32];
    char column_types[32][16];
    Row row;
    Column columns[MAX_COLUMNS];
} Statement;

void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);


typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR
} PrepareResult;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_FAIL
}ExecuteResult;

PrepareResult prepare_statement(const InputBuffer* input_buffer, Statement* statement);
ExecuteResult execute_statement(Statement* statement);
ExecuteResult execute_insert(Statement* statement);
ExecuteResult execute_select(Statement* statement);
ExecuteResult execute_create_table(Statement* statement);
void print_row(Row* row);




#endif