#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "parser.h"

ParseResult parse_select_table(Lexer* lexer, SelectStatement* select_statement);
ParseResult parse_condition(Lexer* lexer, Condition* condition);
ParseResult parse_where_conditions(Lexer* lexer, SelectStatement* select_statement);
ParseResult parse_selected_columns(Lexer* col_lexer, const TableSchema* schema, SelectStatement* select_statement);

#endif