#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include "database.h"
#include "table.h"

/*
 * TODO: Apply both types of the INSERT INTO statements
 *  INSERT INTO table_name (column1, column2, column3, ...) VALUES (value1, value2, value3, ...);
 *  INSERT INTO table_name VALUES (value1, value2, value3, ...);
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

    SelectStatement select_statement;
    select_statement.selected_col_count = 0;

    Lexer selected_col_lexer = *lexer;
    token = next_token(lexer);

    while (token.type != TOKEN_FROM) {
        token = next_token(lexer);
    }

    token = next_token(lexer);
    strncpy(select_statement.table_name, token.text, sizeof(select_statement.table_name));

    const Table* table = find_table(&global_db, token.text);
    if (table == NULL) {
        return PREPARE_TABLE_NOT_FOUND_ERROR;
    }
    const TableSchema schema = table->schema;


    token = next_token(lexer);
    select_statement.condition_count = 0;
    select_statement.has_condition = 0;
    if (token.type == TOKEN_WHERE) {
        while (1) {

            select_statement.conditions[select_statement.condition_count].column_name = NULL;
            select_statement.conditions[select_statement.condition_count].value = NULL;

            token = next_token(lexer);
            if (token.type != TOKEN_IDENTIFIER) {
                free_conditions(select_statement.condition_count + 1, select_statement.conditions);
                return PREPARE_SYNTAX_ERROR;
            }
            select_statement.conditions[select_statement.condition_count].column_name = strdup(token.text);

            token = next_token(lexer);
            switch (token.type) {
                case TOKEN_EQUAL:
                    select_statement.conditions[select_statement.condition_count].type = TOKEN_EQUAL;
                    break;
                case TOKEN_GREATER:
                    select_statement.conditions[select_statement.condition_count].type = TOKEN_GREATER;
                    break;
                case TOKEN_LESS:
                    select_statement.conditions[select_statement.condition_count].type = TOKEN_LESS;
                    break;
                case TOKEN_GREATER_EQUAL:
                    select_statement.conditions[select_statement.condition_count].type = TOKEN_GREATER_EQUAL;
                    break;
                case TOKEN_LESSER_EQUAL:
                    select_statement.conditions[select_statement.condition_count].type = TOKEN_LESSER_EQUAL;
                    break;
                case TOKEN_NOT_EQUAL:
                    select_statement.conditions[select_statement.condition_count].type = TOKEN_NOT_EQUAL;
                    break;
                default:
                    free_conditions(select_statement.condition_count + 1, select_statement.conditions);
                    return PREPARE_SYNTAX_ERROR;
            }


            token = next_token(lexer);
            if (token.type != TOKEN_NUMBER && token.type != TOKEN_STRING) {
                free_conditions(select_statement.condition_count + 1, select_statement.conditions);
                return PREPARE_SYNTAX_ERROR;
            }
            select_statement.conditions[select_statement.condition_count].value = strdup(token.text);

            select_statement.condition_count++;

            token = next_token(lexer);
            if (token.type == TOKEN_SEMICOLON || token.type == TOKEN_EOF) break;
            if (token.type == TOKEN_AND) continue;

            free_conditions(select_statement.condition_count, select_statement.conditions);
            return PREPARE_SYNTAX_ERROR;


        }
        select_statement.has_condition = 1;
    }


    Token col_token = next_token(&selected_col_lexer);
    if (col_token.type != TOKEN_STAR) {
        uint32_t col_index = 0;
        while (col_token.type != TOKEN_FROM) {
            if (col_token.type != TOKEN_IDENTIFIER) {
                free_conditions(select_statement.condition_count, select_statement.conditions);
                return PREPARE_SYNTAX_ERROR;
            }
            for (uint32_t i = 0; i < schema.num_columns; i++ ) {
                if (strcmp(schema.columns[i].name, col_token.text) == 0) {
                    select_statement.selected_col_indexes[col_index] = i;
                    col_index++;
                }

                if (select_statement.has_condition) {
                    for (uint32_t j = 0; j < select_statement.condition_count; j++ ) {
                        if (strcmp(schema.columns[i].name, select_statement.conditions[j].column_name) == 0) {
                            select_statement.conditions[j].column_index = i;
                        }
                    }
                }
            }
            col_token = next_token(&selected_col_lexer);

            if (col_token.type == TOKEN_FROM) break;
            if (col_token.type != TOKEN_COMMA) {
                free_conditions(select_statement.condition_count, select_statement.conditions);
                return PREPARE_SYNTAX_ERROR;
            }

            col_token = next_token(&selected_col_lexer);
        }

        select_statement.selected_col_count = col_index;
    }
    else {
        select_statement.selected_col_count = 0;
        for (uint32_t i = 0; i < schema.num_columns; i++ ) {

            if (select_statement.has_condition) {
                for (uint32_t j = 0; j < select_statement.condition_count; j++ ) {
                    if (strcmp(schema.columns[i].name, select_statement.conditions[j].column_name) == 0) {
                        select_statement.conditions[j].column_index = i;
                    }
                }
            }
        }

    }


    statement->type = STATEMENT_SELECT;
    statement->select_stmt = select_statement;

    return PREPARE_SUCCESS;
    }


PrepareResult parse_create(Lexer* lexer, Statement* statement, const InputBuffer* input_buffer, Token token) {

    token = next_token(lexer);

    if (token.type == TOKEN_TABLE) {
        CreateTableStatement create_statement;
        create_statement.primary_col_index = -1;

        token = next_token(lexer);
        if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
        strncpy(create_statement.table_name, token.text, sizeof(create_statement.table_name));

        token = next_token(lexer);
        if (token.type != TOKEN_OPEN_PAREN) return PREPARE_SYNTAX_ERROR;

        char* open_paren = strchr(input_buffer->buffer, '(');
        if (!open_paren) return PREPARE_SYNTAX_ERROR;

        if (open_paren != strstr(input_buffer->buffer, token.text)) {
            return PREPARE_SYNTAX_ERROR;
        }

        const char* close_paren = find_close_parenthesis(open_paren);
        if (!close_paren) return PREPARE_SYNTAX_ERROR;


        char column_definitions[256];
        strncpy(column_definitions, open_paren+ 1, close_paren - open_paren - 1);
        column_definitions[close_paren - open_paren - 1] = '\0';

        create_statement.num_columns = 0;
        while (1) {
            token = next_token(lexer);

            if (token.type == TOKEN_IDENTIFIER) {
                strcpy(create_statement.columns[create_statement.num_columns].name, token.text);
                create_statement.columns[create_statement.num_columns].index = create_statement.num_columns;

                token = next_token(lexer);
                if (token.type == TOKEN_INT) {
                    create_statement.columns[create_statement.num_columns].type = COLUMN_INT;
                    create_statement.num_columns++;
                }

                else if (token.type == TOKEN_VARCHAR) {

                    token = next_token(lexer);

                    if (token.type != TOKEN_OPEN_PAREN) return PREPARE_SYNTAX_ERROR;

                    token = next_token(lexer);

                    if (token.type != TOKEN_NUMBER) return PREPARE_SYNTAX_ERROR;

                    char *endptr;
                    const long val = strtol(token.text, &endptr, 10);
                    if (endptr == token.text || *endptr != '\0' || val > UINT32_MAX) return PREPARE_SYNTAX_ERROR;

                    uint32_t size = (uint32_t)val;

                    create_statement.columns[create_statement.num_columns].type = COLUMN_VARCHAR;
                    create_statement.columns[create_statement.num_columns].size = size;

                    token = next_token(lexer);
                    if (token.type != TOKEN_CLOSE_PAREN) return PREPARE_SYNTAX_ERROR;
                    create_statement.num_columns++;


                }
            }

            else if (token.type == TOKEN_PRIMARY) {
                token = next_token(lexer);
                if (token.type != TOKEN_KEY) return PREPARE_SYNTAX_ERROR;

                token = next_token(lexer);
                if (token.type != TOKEN_OPEN_PAREN) return PREPARE_SYNTAX_ERROR;


                token = next_token(lexer);
                if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;


                create_statement.primary_col_index = create_statement.num_columns;

                token = next_token(lexer);
                if (token.type != TOKEN_CLOSE_PAREN) return PREPARE_SYNTAX_ERROR;

            }


            token = next_token(lexer);

            if (token.type == TOKEN_CLOSE_PAREN) break;
            if (token.type != TOKEN_COMMA) return PREPARE_SYNTAX_ERROR;
        }

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

    token = next_token(lexer);
    if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;

    const Table* table = find_table(&global_db, token.text);
    if (table == NULL) {
        return PREPARE_TABLE_NOT_FOUND_ERROR;
    }
    const TableSchema schema = table->schema;

    strncpy(delete_statement.table_name, token.text, sizeof(delete_statement.table_name) - 1);
    delete_statement.table_name[sizeof(delete_statement.table_name) - 1] = '\0';

    token = next_token(lexer);
    if (token.type == TOKEN_EOF || token.type == TOKEN_SEMICOLON) {
        statement->type = STATEMENT_DELETE;
        statement->delete_stmt = delete_statement;
        return PREPARE_SUCCESS;
    }

    if (token.type == TOKEN_WHERE) {
        while (1) {

            delete_statement.conditions[delete_statement.condition_count].column_name = NULL;
            delete_statement.conditions[delete_statement.condition_count].value = NULL;

            token = next_token(lexer);
            if (token.type != TOKEN_IDENTIFIER) {
                free_conditions(delete_statement.condition_count + 1, delete_statement.conditions);
                return PREPARE_SYNTAX_ERROR;
            }

            delete_statement.conditions[delete_statement.condition_count].column_name = strdup(token.text);

            token = next_token(lexer);

            switch (token.type) {
                case TOKEN_EQUAL:
                    delete_statement.conditions[delete_statement.condition_count].type = TOKEN_EQUAL;
                    break;
                case TOKEN_GREATER:
                    delete_statement.conditions[delete_statement.condition_count].type = TOKEN_GREATER;
                    break;
                case TOKEN_LESS:
                    delete_statement.conditions[delete_statement.condition_count].type = TOKEN_LESS;
                    break;
                case TOKEN_GREATER_EQUAL:
                    delete_statement.conditions[delete_statement.condition_count].type = TOKEN_GREATER_EQUAL;
                    break;
                case TOKEN_LESSER_EQUAL:
                    delete_statement.conditions[delete_statement.condition_count].type = TOKEN_LESSER_EQUAL;
                    break;
                case TOKEN_NOT_EQUAL:
                    delete_statement.conditions[delete_statement.condition_count].type = TOKEN_NOT_EQUAL;
                    break;
                default:
                    free_conditions(delete_statement.condition_count + 1, delete_statement.conditions);
                    return PREPARE_SYNTAX_ERROR;
            }


            token = next_token(lexer);
            if (token.type != TOKEN_NUMBER && token.type != TOKEN_STRING){
                free_conditions(delete_statement.condition_count + 1, delete_statement.conditions);
                return PREPARE_SYNTAX_ERROR;
            }
            delete_statement.conditions[delete_statement.condition_count].value = strdup(token.text);


            delete_statement.condition_count++;

            token = next_token(lexer);
            if (token.type == TOKEN_SEMICOLON || token.type == TOKEN_EOF) break;
            if (token.type == TOKEN_AND) continue;

            free_conditions(delete_statement.condition_count, delete_statement.conditions);
            return PREPARE_SYNTAX_ERROR;

        }
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