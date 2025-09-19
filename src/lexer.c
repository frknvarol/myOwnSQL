#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"

void init_lexer(Lexer *lexer, const char *input) {
    lexer->input = input;
    lexer->pos = 0;
    lexer->length = strlen(input);
}

char peek(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    return lexer->input[lexer->pos];
}

char advance(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    return lexer->input[lexer->pos++];
}

void skip_whitespace(Lexer *lexer) {
    while (isspace(peek(lexer))) advance(lexer);
}

int is_keyword(const char *str, TokenType *type) {
    if (strcasecmp(str, "SELECT") == 0) { *type = TOKEN_SELECT; return 1; }
    if (strcasecmp(str, "FROM") == 0)   { *type = TOKEN_FROM; return 1; }
    if (strcasecmp(str, "WHERE") == 0)  { *type = TOKEN_WHERE; return 1; }
    if (strcasecmp(str, "INSERT") == 0) { *type = TOKEN_INSERT; return 1; }
    if (strcasecmp(str, "INTO") == 0)   { *type = TOKEN_INTO; return 1; }
    if (strcasecmp(str, "VALUES") == 0) { *type = TOKEN_VALUES; return 1; }
    if (strcasecmp(str, "CREATE") == 0) { *type = TOKEN_CREATE; return 1; }
    if (strcasecmp(str, "TABLE") == 0) { *type = TOKEN_TABLE; return 1; }
    if (strcasecmp(str, "DATABASE") == 0) { *type = TOKEN_DATABASE; return 1; }
    if (strcasecmp(str, "PRIMARY") == 0) { *type = TOKEN_PRIMARY; return 1; }
    if (strcasecmp(str, "AND") == 0) { *type = TOKEN_AND; return 1; }
    if (strcasecmp(str, "KEY") == 0) { *type = TOKEN_KEY; return 1; }
    if (strncasecmp(str, "VARCHAR", 7) == 0) { *type = TOKEN_VARCHAR; return 1; }
    if (strncasecmp(str, "INT", 3) == 0) { *type = TOKEN_INT; return 1; }


    return 0;
}

Token make_token(const TokenType type, const char *text) {
    Token token;
    token.type = type;
    strncpy(token.text, text, sizeof(token.text));
    token.text[sizeof(token.text)-1] = '\0';
    return token;
}

Token next_token(Lexer *lexer) {
    skip_whitespace(lexer);
    char chr = peek(lexer);
    if (chr == '\0') return make_token(TOKEN_EOF, "");

    if (chr == ',') { advance(lexer); return make_token(TOKEN_COMMA, ","); }
    if (chr == ';') { advance(lexer); return make_token(TOKEN_SEMICOLON, ";"); }
    if (chr == '*') { advance(lexer); return make_token(TOKEN_STAR, "*"); }
    if (chr == '=') { advance(lexer); return make_token(TOKEN_EQUAL, "="); }
    if (chr == '>') { advance(lexer); return make_token(TOKEN_GREATER, ">"); }
    if (chr == '<') { advance(lexer); return make_token(TOKEN_LESS, "<"); }
    if (chr == '(') { advance(lexer); return make_token(TOKEN_OPEN_PAREN, "("); }
    if (chr == ')') { advance(lexer); return make_token(TOKEN_CLOSE_PAREN, ")"); }


    if (chr == '\'' || chr == '"') {
        char quote = advance(lexer);
        char buffer[256]; size_t i = 0;
        while (peek(lexer) != quote && peek(lexer) != '\0' && i < sizeof(buffer)-1)
            buffer[i++] = advance(lexer);
        if (peek(lexer) == quote) advance(lexer); // consume closing quote
        buffer[i] = '\0';
        return make_token(TOKEN_STRING, buffer);
    }


    if (isdigit(chr)) {
        char buffer[256]; size_t i = 0;
        while (isdigit(peek(lexer)) && i < sizeof(buffer)-1)
            buffer[i++] = advance(lexer);
        buffer[i] = '\0';
        return make_token(TOKEN_NUMBER, buffer);
    }


    if (isalpha(chr)) {
        char buffer[256]; size_t i = 0;
        while ((isalnum(peek(lexer)) || peek(lexer) == '_') && i < sizeof(buffer)-1)
            buffer[i++] = advance(lexer);
        buffer[i] = '\0';
        TokenType type;
        if (is_keyword(buffer, &type))
            return make_token(type, buffer);
        return make_token(TOKEN_IDENTIFIER, buffer);
    }


    advance(lexer);
    return make_token(TOKEN_UNKNOWN, (char[]){chr, '\0'});
}

