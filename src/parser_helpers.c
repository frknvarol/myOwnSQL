#include "parser_helpers.h"

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