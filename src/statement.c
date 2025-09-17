#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "statement.h"

#include <stdbool.h>

#include "input_buffer.h"
#include "table.h"
#include "database.h"
#include "lexer.h"

Database global_db;


PrepareResult prepare_statement(const InputBuffer* input_buffer, Statement* statement) {

    Lexer lexer;
    init_lexer(&lexer, input_buffer->buffer);
    Token token = next_token(&lexer);

    if (token.type == TOKEN_INSERT) {
        token = next_token(&lexer);
        if (token.type != TOKEN_INTO) return PREPARE_SYNTAX_ERROR;

        token = next_token(&lexer);
        if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
        strncpy(statement->table_name, token.text, sizeof(statement->table_name));

        const Table* table = find_table(&global_db, token.text);
        const TableSchema schema = table->schema;
        const Row* row = create_row(&table->schema);
        statement->row = *row;


        token = next_token(&lexer);
        if (token.type != TOKEN_VALUES) return PREPARE_SYNTAX_ERROR;

        token = next_token(&lexer);
        if (token.type != TOKEN_OPEN_PAREN || strcmp(token.text, "(") != 0)
            return PREPARE_SYNTAX_ERROR;


        int col_index = 0;
        while (1) {
            token = next_token(&lexer);

            const ColumnType column_type = schema.columns[col_index].type;

            switch (column_type) {
                case COLUMN_INT: {
                    char *endptr;

                    const long int num = strtol(token.text, &endptr, 10);
                    if (endptr == token.text || *endptr != '\0' || token.type != TOKEN_NUMBER) {
                        return PREPARE_INSERT_TYPE_ERROR;
                    }
                    set_int_value(&table->schema, &statement->row, col_index, (int32_t)num);
                    break;
                }
                case COLUMN_VARCHAR: {
                    if (token.type != TOKEN_STRING) return PREPARE_INSERT_TYPE_ERROR;
                    if (schema.columns[col_index].size < strlen(token.text)) return PREPARE_INSERT_VARCHAR_SIZE_ERROR;
                    set_text_value(&table->schema, &statement->row, col_index, token.text);
                    break;
                }
                default:
                    return PREPARE_SYNTAX_ERROR;
            }

            col_index++;

            const Token next = next_token(&lexer);
            if (strcmp(next.text, ")") == 0) break;
            if (next.type != TOKEN_COMMA) return PREPARE_SYNTAX_ERROR;
        }

        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }

    if (token.type == TOKEN_SELECT) {
        Token col = next_token(&lexer); // e.g. '*' or column
        const Token from = next_token(&lexer);
        if (from.type != TOKEN_FROM) return PREPARE_SYNTAX_ERROR;

        const Token table = next_token(&lexer);
        strncpy(statement->table_name, table.text, sizeof(statement->table_name));

        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }


    if (token.type == TOKEN_CREATE) {
        token = next_token(&lexer);
        if (token.type != TOKEN_TABLE) return PREPARE_SYNTAX_ERROR;

        token = next_token(&lexer);
        if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
        strncpy(statement->table_name, token.text, sizeof(statement->table_name));

        token = next_token(&lexer);
        if (token.type != TOKEN_OPEN_PAREN) return PREPARE_SYNTAX_ERROR;

        char* open_paren = strchr(input_buffer->buffer, '(');
        if (!open_paren) return PREPARE_SYNTAX_ERROR;

        if (open_paren != strstr(input_buffer->buffer, token.text)) {
            return PREPARE_SYNTAX_ERROR;
        }

        char* close_paren = find_close_parenthesis(open_paren);
        if (!close_paren) return PREPARE_SYNTAX_ERROR;


        char column_definitions[256];
        strncpy(column_definitions, open_paren+ 1, close_paren - open_paren - 1);
        column_definitions[close_paren - open_paren - 1] = '\0';

        statement->num_columns = 0;
        while (true) {
            token = next_token(&lexer);
            if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
            strcpy(statement->columns[statement->num_columns].name, token.text);

            token = next_token(&lexer);
            if (token.type == TOKEN_INT) {
                statement->columns[statement->num_columns].type = COLUMN_INT;

            }

            else if (token.type == TOKEN_VARCHAR) {

                token = next_token(&lexer);

                if (token.type != TOKEN_OPEN_PAREN) return PREPARE_SYNTAX_ERROR;

                token = next_token(&lexer);

                if (token.type != TOKEN_NUMBER) return PREPARE_SYNTAX_ERROR;

                char *endptr;
                const long val = strtol(token.text, &endptr, 10);
                if (endptr == token.text || *endptr != '\0' || val > UINT32_MAX) return PREPARE_SYNTAX_ERROR;

                uint32_t size = (uint32_t)val;

                statement->columns[statement->num_columns].type = COLUMN_VARCHAR;
                statement->columns[statement->num_columns].size = size;

                token = next_token(&lexer);

                if (token.type != TOKEN_CLOSE_PAREN) return PREPARE_SYNTAX_ERROR;

            }

            token = next_token(&lexer);

            statement->num_columns++;
            if (token.type == TOKEN_CLOSE_PAREN) break;
            if (token.type != TOKEN_COMMA) return PREPARE_SYNTAX_ERROR;
        }

        statement->type = STATEMENT_CREATE_TABLE;

        return PREPARE_SUCCESS;

    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}


void serialize_row(const TableSchema* schema, const Row* source, void* destination) {
    size_t offset = 0;
    for (int i = 0; i < schema->num_columns; i++) {
        size_t col_size = (schema->columns[i].type == COLUMN_INT) ? sizeof(int32_t) : 256;
        memcpy((char*)destination + offset, source->data + offset, col_size);
        offset += col_size;
    }
}

void deserialize_row(const TableSchema* schema, void* source, Row* destination) {
    size_t offset = 0;
    for (int i = 0; i < schema->num_columns; i++) {
        size_t col_size = (schema->columns[i].type == COLUMN_INT) ? sizeof(int32_t) : 256;
        memcpy(destination->data + offset, (char*)source + offset, col_size);
        offset += col_size;
    }
}


void* row_slot(Table* table, uint32_t row_num) {
    size_t row_size = compute_row_size(&table->schema);
    size_t rows_per_page = PAGE_SIZE / row_size;

    size_t page_num = row_num / rows_per_page;
    if (table->pages[page_num] == NULL) {
        table->pages[page_num] = malloc(PAGE_SIZE);
        if (!table->pages[page_num]) {
            perror("malloc failed");
            exit(1);
        }
    }
    void* page = table->pages[page_num];


    size_t row_offset = row_num % rows_per_page;
    size_t byte_offset = row_offset * row_size;
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
    void* destination = row_slot(table, table->num_rows);

    serialize_row(&table->schema, row_to_insert, destination);
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement) {
    Table* table = find_table(&global_db, statement->table_name);
    Row row;
    row.data = malloc(compute_row_size(&table->schema));
    for (uint32_t i = 0; i < table->num_rows; i++) {
        deserialize_row(&table->schema, row_slot(table, i), &row);
        print_row(&table->schema, &row);
    }
    free(row.data);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_create_table(const Statement* statement) {
    Table* new_table = malloc(sizeof(Table));
    if (!new_table) {
        printf("Error: memory allocation failed.\n");
        return EXECUTE_FAIL;
    }

    strncpy(new_table->name, statement->table_name, sizeof(new_table->name));
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
           new_table->name, new_table->schema.num_columns);
    return EXECUTE_SUCCESS;
}


void print_row(const TableSchema* schema, const Row* row) {
    printf("(");
    for (int i = 0; i < schema->num_columns; i++) {
        if (i > 0) printf(", ");
        if (schema->columns[i].type == COLUMN_INT) {
            int32_t val;
            memcpy(&val, row->data + get_column_offset(schema, i), sizeof(int32_t));
            printf("%d", val);
        } else if (schema->columns[i].type == COLUMN_VARCHAR){
            char buf[257];
            memcpy(buf, row->data + get_column_offset(schema, i), 256);
            buf[256] = '\0';
            printf("%s", buf);
        }
    }
    printf(")\n");
}



char* find_close_parenthesis(char* open_parenthesis) {
    /*Function to find the closing parenthesis of an open parenthesis*/
    int open_paren_count = 0;

    while (*open_parenthesis != '\0') {
        open_parenthesis++;

        if (*open_parenthesis == '(') {
            open_paren_count++;
        } else if (*open_parenthesis == ')') {
            if (open_paren_count == 0) {
                return open_parenthesis;
            }
            open_paren_count--;
        }
    }

    return NULL;
}
