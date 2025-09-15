#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_SELECT, TOKEN_FROM, TOKEN_WHERE,
    TOKEN_INSERT, TOKEN_INTO, TOKEN_VALUES,
    TOKEN_CREATE, TOKEN_TABLE,
    TOKEN_INT, TOKEN_VARCHAR,
    TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_STRING,
    TOKEN_COMMA, TOKEN_SEMICOLON, TOKEN_STAR, TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN,
    TOKEN_EQUAL, TOKEN_GREATER, TOKEN_LESS,
    TOKEN_UNKNOWN, TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char text[256];
} Token;

typedef struct {
    const char *input;
    size_t pos;
    size_t length;
} Lexer;

void init_lexer(Lexer *lexer, const char *input);
char peek(Lexer *lexer);
char advance(Lexer *lexer);
void skip_whitespace(Lexer *lexer);
int is_keyword(const char *str, TokenType *type);
Token make_token(TokenType type, const char *text);
Token next_token(Lexer *lexer);

#endif