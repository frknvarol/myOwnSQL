#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "database.h"
#include "table.h"
#include "parser_helpers.h"

/*
 * TODO: Apply both types of the INSERT INTO statements
 *  INSERT INTO table_name (column1, column2, column3, ...) VALUES (value1, value2, value3, ...);
 *  INSERT INTO table_name VALUES (value1, value2, value3, ...);
 *  make two smaller functions to parse the two different options
 */
PrepareResult parse_insert(Lexer* lexer, Statement* statement, Token token) {
    InsertStatement insert_statement;
    token = next_token(lexer);
    if (token.type != TOKEN_INTO) return PREPARE_SYNTAX_ERROR;

    token = next_token(lexer);
    if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
    strncpy(insert_statement.table_name, token.text, sizeof(insert_statement.table_name));
    const Table* table = find_table(&global_db, token.text);
    if (table == NULL) {
        return PREPARE_TABLE_NOT_FOUND_ERROR;
    }
    const TableSchema schema = table->schema;

    const Row* row = create_row(&table->schema);
    insert_statement.row = *row;


    token = next_token(lexer);
    if (token.type != TOKEN_VALUES) return PREPARE_SYNTAX_ERROR;

    token = next_token(lexer);
    if (token.type != TOKEN_OPEN_PAREN || strcmp(token.text, "(") != 0)
        return PREPARE_SYNTAX_ERROR;


    int col_index = 0;
    while (1) {
        token = next_token(lexer);

        const ColumnType column_type = schema.columns[col_index].type;

        switch (column_type) {
            case COLUMN_INT: {
                char *endptr;

                const long int num = strtol(token.text, &endptr, 10);
                if (endptr == token.text || *endptr != '\0' || token.type != TOKEN_NUMBER) {
                    return PREPARE_INSERT_TYPE_ERROR;
                }
                set_int_value(&table->schema, &insert_statement.row, col_index, (int32_t)num);
                break;
            }
            case COLUMN_VARCHAR: {
                if (token.type != TOKEN_STRING) return PREPARE_INSERT_TYPE_ERROR;
                if (schema.columns[col_index].size < strlen(token.text)) return PREPARE_INSERT_VARCHAR_SIZE_ERROR;
                set_text_value(&table->schema, &insert_statement.row, col_index, token.text);
                break;
            }
            default:
                return PREPARE_SYNTAX_ERROR;
        }

        col_index++;

        const Token next = next_token(lexer);
        if (strcmp(next.text, ")") == 0) break;
        if (next.type != TOKEN_COMMA) return PREPARE_SYNTAX_ERROR;
    }

    statement->type = STATEMENT_INSERT;
    statement->insert_stmt = insert_statement;
    return PREPARE_SUCCESS;
}

PrepareResult parse_select(Lexer* lexer, Statement* statement, Token token) {

    SelectStatement select_statement = {0};

    Lexer selected_col_lexer = *lexer;
    token = next_token(lexer);

    while (token.type != TOKEN_FROM) token = next_token(lexer);


    if (parse_table_name(lexer, select_statement.table_name, sizeof(select_statement.table_name)) != PARSE_SUCCESS)
        return PREPARE_SYNTAX_ERROR;

    const Table* table = find_table(&global_db, select_statement.table_name);
    if (table == NULL) {
        return PREPARE_TABLE_NOT_FOUND_ERROR;
    }
    const TableSchema schema = table->schema;

    token = next_token(lexer);
    if (token.type == TOKEN_WHERE) {
        if (parse_where_conditions(lexer, &select_statement.condition_count, select_statement.conditions) != PARSE_SUCCESS)
            return PREPARE_SYNTAX_ERROR;
        select_statement.has_condition = 1;
    }


    if (parse_selected_columns(&selected_col_lexer, &schema, &select_statement) != PARSE_SUCCESS) {
        free_conditions(select_statement.condition_count, select_statement.conditions);
        return PREPARE_SYNTAX_ERROR;
    }

    if (select_statement.has_condition) {
        for (uint32_t condition_index = 0; condition_index < select_statement.condition_count; condition_index++) {
            const int col_index = get_column_index(&schema, select_statement.conditions[condition_index].column_name);
            if (col_index < 0) {
                free_conditions(select_statement.condition_count, select_statement.conditions);
                return PREPARE_SYNTAX_ERROR;
            }
            select_statement.conditions[condition_index].column_index = col_index;
        }
    }

    statement->type = STATEMENT_SELECT;
    statement->select_stmt = select_statement;
    return PREPARE_SUCCESS;
}

PrepareResult parse_create(Lexer* lexer, Statement* statement, const InputBuffer* input_buffer, Token token) {

    token = next_token(lexer);

    if (token.type == TOKEN_TABLE) {
        CreateTableStatement create_statement = {0};
        create_statement.primary_col_index = -1;

        if (parse_create_table_name(lexer, &create_statement) != PARSE_SUCCESS)
            return PREPARE_SYNTAX_ERROR;

        if (parse_open_paren(lexer) != PARSE_SUCCESS)
            return PREPARE_SYNTAX_ERROR;

        char column_definitions[256];
        const char* open_paren;
        const char* close_paren;

        if (extract_column_definitions(input_buffer, column_definitions, sizeof(column_definitions),
                                       &open_paren, &close_paren) != PARSE_SUCCESS)
            return PREPARE_SYNTAX_ERROR;

        if (parse_columns(lexer, &create_statement) != PARSE_SUCCESS)
            return PREPARE_SYNTAX_ERROR;

        statement->type = STATEMENT_CREATE_TABLE;
        statement->create_table_stmt = create_statement;
        return PREPARE_SUCCESS;
    }

    //TODO add searching and selecting a database
    /*
    if (token.type == TOKEN_DATABASE) {
        token = next_token(lexer);

        Database db;
        init_database(&db, token.text);

    }
    */

    return PREPARE_UNRECOGNIZED_STATEMENT;

}

PrepareResult parse_drop(Lexer* lexer, Statement* statement, Token token) {
    token = next_token(lexer);
    if (token.type != TOKEN_TABLE) return PREPARE_SYNTAX_ERROR;
    token = next_token(lexer);

    DropTableStatement drop_table_statement;

    strncpy(drop_table_statement.table_name, token.text, sizeof(drop_table_statement.table_name) - 1);
    drop_table_statement.table_name[sizeof(drop_table_statement.table_name) - 1] = '\0';

    statement->type = STATEMENT_DROP_TABLE;
    statement->drop_table_stmt = drop_table_statement;
    return PREPARE_SUCCESS;
}

PrepareResult parse_show(Lexer* lexer, Statement* statement, Token token) {
    token = next_token(lexer);
    if (token.type != TOKEN_TABLES) return PREPARE_SYNTAX_ERROR;
    token = next_token(lexer);
    if (token.type != TOKEN_EOF && token.type != TOKEN_SEMICOLON) return PREPARE_SYNTAX_ERROR;

    statement->type = STATEMENT_SHOW_TABLES;
    return PREPARE_SUCCESS;

}

PrepareResult parse_delete(Lexer* lexer, Statement* statement, Token token) {
    DeleteStatement delete_statement;
    delete_statement.condition_count = 0;
    delete_statement.has_condition = 0;

    token = next_token(lexer);
    if (token.type != TOKEN_FROM) return PREPARE_SYNTAX_ERROR;

    if (parse_table_name(lexer, delete_statement.table_name, sizeof(delete_statement.table_name)) != PARSE_SUCCESS)
        return PREPARE_SYNTAX_ERROR;

    const Table* table = find_table(&global_db, delete_statement.table_name);
    if (table == NULL) {
        return PREPARE_TABLE_NOT_FOUND_ERROR;
    }
    const TableSchema schema = table->schema;


    token = next_token(lexer);
    if (token.type == TOKEN_EOF || token.type == TOKEN_SEMICOLON) {
        statement->type = STATEMENT_DELETE;
        statement->delete_stmt = delete_statement;
        return PREPARE_SUCCESS;
    }

    if (token.type == TOKEN_WHERE) {
        if (parse_where_conditions(lexer, &delete_statement.condition_count, delete_statement.conditions) != PARSE_SUCCESS)
            return PREPARE_SYNTAX_ERROR;
        delete_statement.has_condition = 1;
    }




    if (delete_statement.has_condition) {
        for (int32_t j = 0; j < delete_statement.condition_count; j++) {
            const int32_t col_index = get_column_index(&schema, delete_statement.conditions[j].column_name);
            if (col_index < 0) {
                free_conditions(delete_statement.condition_count, delete_statement.conditions);
                return PREPARE_SYNTAX_ERROR;
            }

            delete_statement.conditions[j].column_index = col_index;
        }
    }
    statement->type = STATEMENT_DELETE;
    statement->delete_stmt = delete_statement;
    return PREPARE_SUCCESS;
}