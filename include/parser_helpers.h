#ifndef PARSER_HELPERS_H
#define PARSER_HELPERS_H

#include "parser.h"

ParseResult parse_table_name(Lexer* lexer, char* table_name, size_t size);
ParseResult parse_condition(Lexer* lexer, Condition* condition);
ParseResult parse_where_conditions(Lexer* lexer, uint32_t* condition_count, Condition* conditions);
ParseResult parse_selected_columns(Lexer* col_lexer, const TableSchema* schema, SelectStatement* select_statement);
ParseResult parse_create_table_name(Lexer* lexer, CreateTableStatement* create_statement);
ParseResult parse_open_paren(Lexer* lexer);
ParseResult extract_column_definitions(const InputBuffer* buffer, char* out, size_t out_size, const char** open_paren, const char** close_paren);
ParseResult parse_create_table_columns(Lexer* lexer, CreateTableStatement* create_statement);
ParseResult parse_create_table_column(Lexer* lexer, CreateTableStatement* create_statement);
ParseResult parse_primary_key(Lexer* lexer, CreateTableStatement* create_statement);
ParseResult parse_columns(Lexer* lexer, CreateTableStatement* create_statement);

#endif