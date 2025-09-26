#ifndef PARSER_HELPERS_H
#define PARSER_HELPERS_H

#include "parser.h"

ParseResult parse_select_table(Lexer* lexer, SelectStatement* select_statement);
ParseResult parse_condition(Lexer* lexer, Condition* condition);
ParseResult parse_where_conditions(Lexer* lexer, uint32_t* condition_count, Condition* conditions);
ParseResult parse_selected_columns(Lexer* col_lexer, const TableSchema* schema, SelectStatement* select_statement);

#endif