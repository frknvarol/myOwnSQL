#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "statement.h"

PrepareResult parse_insert(Lexer* lexer, Statement* statement, Token token);
PrepareResult parse_select(Lexer* lexer, Statement* statement, Token token);
PrepareResult parse_create(Lexer* lexer, Statement* statement, const InputBuffer* input_buffer, Token token);
PrepareResult parse_drop(Lexer* lexer, Statement* statement, Token token);
PrepareResult parse_show(Lexer* lexer, Statement* statement, Token token);
PrepareResult parse_delete(Lexer* lexer, Statement* statement, Token token);

#endif
