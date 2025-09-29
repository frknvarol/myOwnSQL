#include "parser_helpers.h"

#include <stdlib.h>
#include <string.h>
#include <_string.h>

ParseResult parse_table_name(Lexer* lexer, char* table_name, size_t size) {
    const Token token = next_token(lexer);
    if (token.type != TOKEN_IDENTIFIER) return PARSE_SYNTAX_ERROR;

    strncpy(table_name, token.text, size);
    table_name[size - 1] = '\0';  // ensure null termination

    return PARSE_SUCCESS;
}

ParseResult parse_condition(Lexer* lexer, Condition* condition) {
    Token token = next_token(lexer);
    if (token.type != TOKEN_IDENTIFIER) return PARSE_SYNTAX_ERROR;
    condition->column_name = strdup(token.text);

    token = next_token(lexer);
    switch (token.type) {
        case TOKEN_EQUAL: condition->type = TOKEN_EQUAL; break;
        case TOKEN_GREATER: condition->type = TOKEN_GREATER; break;
        case TOKEN_LESS: condition->type = TOKEN_LESS; break;
        case TOKEN_GREATER_EQUAL: condition->type = TOKEN_GREATER_EQUAL; break;
        case TOKEN_LESSER_EQUAL: condition->type = TOKEN_LESSER_EQUAL; break;
        case TOKEN_NOT_EQUAL: condition->type = TOKEN_NOT_EQUAL; break;
        default: return PARSE_SYNTAX_ERROR;
    }

    token = next_token(lexer);
    if (token.type != TOKEN_NUMBER && token.type != TOKEN_STRING)
        return PARSE_SYNTAX_ERROR;
    condition->value = strdup(token.text);

    return PARSE_SUCCESS;
}
ParseResult parse_where_conditions(Lexer* lexer, uint32_t* condition_count, Condition* conditions){

    while (1) {
        Condition* condition = &conditions[*condition_count];
        condition->column_name = NULL;
        condition->value = NULL;

        if (parse_condition(lexer, condition) != PARSE_SUCCESS) {
            free_conditions(*condition_count + 1, conditions);
            return PARSE_SYNTAX_ERROR;
        }

        (*condition_count)++;

        const Token token = next_token(lexer);
        if (token.type == TOKEN_SEMICOLON || token.type == TOKEN_EOF) break;
        if (token.type == TOKEN_AND) continue;

        free_conditions(*condition_count, conditions);
        return PARSE_SYNTAX_ERROR;
    }

    return PARSE_SUCCESS;
}


ParseResult parse_selected_columns(Lexer* col_lexer, const TableSchema* schema, SelectStatement* select_statement) {
    Token token = next_token(col_lexer);
    if (token.type == TOKEN_STAR) {
        select_statement->selected_col_count = 0;
        return PARSE_SUCCESS;
    }

    uint32_t col_index = 0;
    while (token.type != TOKEN_FROM) {
        if (token.type != TOKEN_IDENTIFIER) return PARSE_SYNTAX_ERROR;

        int found = 0;
        for (uint32_t i = 0; i < schema->num_columns; i++) {
            if (strcmp(schema->columns[i].name, token.text) == 0) {
                select_statement->selected_col_indexes[col_index++] = i;
                found = 1;
                break;
            }
        }
        if (!found) return PARSE_SYNTAX_ERROR;

        token = next_token(col_lexer);
        if (token.type == TOKEN_FROM) break;
        if (token.type != TOKEN_COMMA) return PARSE_SYNTAX_ERROR;
        token = next_token(col_lexer);
    }

    select_statement->selected_col_count = col_index;
    return PARSE_SUCCESS;
}

ParseResult parse_create_table_name(Lexer* lexer, CreateTableStatement* create_statement) {
    Token token = next_token(lexer);
    if (token.type != TOKEN_IDENTIFIER) return PARSE_SYNTAX_ERROR;

    strncpy(create_statement->table_name, token.text, sizeof(create_statement->table_name));
    create_statement->table_name[sizeof(create_statement->table_name) - 1] = '\0';
    return PARSE_SUCCESS;
}

ParseResult parse_open_paren(Lexer* lexer) {
    const Token token = next_token(lexer);
    return token.type == TOKEN_OPEN_PAREN ? PARSE_SUCCESS : PARSE_SYNTAX_ERROR;
}

ParseResult extract_column_definitions(const InputBuffer* buffer, char* out, const size_t out_size, const char** open_paren, const char** close_paren) {
    *open_paren = strchr(buffer->buffer, '(');
    if (!*open_paren) return PARSE_SYNTAX_ERROR;

    *close_paren = find_close_parenthesis(*open_paren);
    if (!*close_paren) return PARSE_SYNTAX_ERROR;

    size_t len = *close_paren - *open_paren - 1;
    if (len >= out_size) len = out_size - 1;

    strncpy(out, *open_paren + 1, len);
    out[len] = '\0';
    return PARSE_SUCCESS;
}

ParseResult parse_columns(Lexer* lexer, CreateTableStatement* create_statement) {
    create_statement->num_columns = 0;
    return parse_create_table_columns(lexer, create_statement);
}

ParseResult parse_create_table_columns(Lexer* lexer, CreateTableStatement* create_statement) {

    Token token = next_token(lexer);
    while (1) {

        if (token.type == TOKEN_IDENTIFIER) {
            strcpy(create_statement->columns[create_statement->num_columns].name, token.text);
            create_statement->columns[create_statement->num_columns].index = create_statement->num_columns;

            if (parse_create_table_column(lexer, create_statement) != PARSE_SUCCESS) return PARSE_SYNTAX_ERROR;
        }

        else if (token.type == TOKEN_PRIMARY) {
            if (parse_primary_key(lexer, create_statement) != PARSE_SUCCESS) return PARSE_SYNTAX_ERROR;
        }

        token = next_token(lexer);

        if (token.type == TOKEN_CLOSE_PAREN) break;
        if (token.type != TOKEN_COMMA) return PARSE_SYNTAX_ERROR;

        token = next_token(lexer);
    }

    return PARSE_SUCCESS;
}

ParseResult parse_create_table_column(Lexer* lexer, CreateTableStatement* create_statement) {
    Token token = next_token(lexer);
    if (token.type == TOKEN_INT) {
        create_statement->columns[create_statement->num_columns].type = COLUMN_INT;
        create_statement->num_columns++;
    }

    else if (token.type == TOKEN_VARCHAR) {

        token = next_token(lexer);

        if (token.type != TOKEN_OPEN_PAREN) return PARSE_SYNTAX_ERROR;

        token = next_token(lexer);

        if (token.type != TOKEN_NUMBER) return PARSE_SYNTAX_ERROR;

        char *endptr;
        const long val = strtol(token.text, &endptr, 10);
        if (endptr == token.text || *endptr != '\0' || val > UINT32_MAX) return PARSE_SYNTAX_ERROR;

        const uint32_t size = (uint32_t)val;

        create_statement->columns[create_statement->num_columns].type = COLUMN_VARCHAR;
        create_statement->columns[create_statement->num_columns].size = size;

        token = next_token(lexer);
        if (token.type != TOKEN_CLOSE_PAREN) return PARSE_SYNTAX_ERROR;
        create_statement->num_columns++;

    }
    return PARSE_SUCCESS;
}



ParseResult parse_primary_key(Lexer* lexer, CreateTableStatement* create_statement) {
    Token token = next_token(lexer);
    if (token.type != TOKEN_KEY) return PARSE_SYNTAX_ERROR;

    token = next_token(lexer);
    if (token.type != TOKEN_OPEN_PAREN) return PARSE_SYNTAX_ERROR;


    token = next_token(lexer);
    if (token.type != TOKEN_IDENTIFIER) return PARSE_SYNTAX_ERROR;


    create_statement->primary_col_index = create_statement->num_columns;

    token = next_token(lexer);
    if (token.type != TOKEN_CLOSE_PAREN) return PARSE_SYNTAX_ERROR;

    return PARSE_SUCCESS;
}