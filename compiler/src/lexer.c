/*
 * HOLEXA Lexer
 * Converts source text into tokens
 */

#include "holexa.h"

/* ============================================================
 * KEYWORD TABLE
 * ============================================================ */
typedef struct { const char *word; TokenType type; } Keyword;

static const Keyword KEYWORDS[] = {
    {"let",      TOK_LET},
    {"mut",      TOK_MUT},
    {"fn",       TOK_FN},
    {"return",   TOK_RETURN},
    {"if",       TOK_IF},
    {"else",     TOK_ELSE},
    {"while",    TOK_WHILE},
    {"for",      TOK_FOR},
    {"in",       TOK_IN},
    {"loop",     TOK_LOOP},
    {"break",    TOK_BREAK},
    {"continue", TOK_CONTINUE},
    {"struct",   TOK_STRUCT},
    {"impl",     TOK_IMPL},
    {"trait",    TOK_TRAIT},
    {"import",   TOK_IMPORT},
    {"use",      TOK_USE},
    {"pub",      TOK_PUB},
    {"mod",      TOK_MOD},
    {"match",    TOK_MATCH},
    {"enum",     TOK_ENUM},
    {"type",     TOK_TYPE},
    {"async",    TOK_ASYNC},
    {"await",    TOK_AWAIT},
    {"spawn",    TOK_SPAWN},
    {"extern",   TOK_EXTERN},
    {"const",    TOK_CONST},
    {"static",   TOK_STATIC},
    {"true",     TOK_TRUE},
    {"false",    TOK_FALSE},
    {"none",     TOK_NONE},
    {"self",     TOK_SELF},
    {"new",      TOK_NEW},
    {"delete",   TOK_DELETE},
    /* Built-in types */
    {"int",      TOK_INT},
    {"float",    TOK_FLOAT},
    {"String",   TOK_STRING},
    {"bool",     TOK_BOOL},
    {"void",     TOK_VOID},
    {"byte",     TOK_BYTE},
    {"char",     TOK_CHAR},
    {"i8",       TOK_I8},
    {"i16",      TOK_I16},
    {"i32",      TOK_I32},
    {"i64",      TOK_I64},
    {"u8",       TOK_U8},
    {"u16",      TOK_U16},
    {"u32",      TOK_U32},
    {"u64",      TOK_U64},
    {"f32",      TOK_F32},
    {"f64",      TOK_F64},
    {NULL,       TOK_ERROR}
};

/* ============================================================
 * HELPERS
 * ============================================================ */
static char current_char(HolexaCompiler *c) {
    if (c->pos >= (int)c->source_len) return '\0';
    return c->source[c->pos];
}

static char peek_char(HolexaCompiler *c) {
    if (c->pos + 1 >= (int)c->source_len) return '\0';
    return c->source[c->pos + 1];
}

static char advance(HolexaCompiler *c) {
    char ch = c->source[c->pos++];
    if (ch == '\n') { c->line++; c->col = 1; }
    else             { c->col++; }
    return ch;
}

static void skip_whitespace_and_comments(HolexaCompiler *c) {
    while (c->pos < (int)c->source_len) {
        char ch = current_char(c);

        /* Whitespace (not newlines - they are tokens) */
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            advance(c);
            continue;
        }

        /* Line comment // */
        if (ch == '/' && peek_char(c) == '/') {
            while (c->pos < (int)c->source_len && current_char(c) != '\n')
                advance(c);
            continue;
        }

        /* Block comment: slash-star ... star-slash */
        if (ch == '/' && peek_char(c) == '*') {
            advance(c); advance(c); /* consume /  and * */
            while (c->pos < (int)c->source_len) {
                if (current_char(c) == '*' && peek_char(c) == '/') {
                    advance(c); advance(c);
                    break;
                }
                advance(c);
            }
            continue;
        }

        /* Doc comment /// - treat as line comment */
        break;
    }
}

static Token make_token(HolexaCompiler *c, TokenType type, const char *val) {
    Token t;
    t.type     = type;
    t.line     = c->line;
    t.col      = c->col;
    t.filename = c->filename;
    strncpy(t.value, val ? val : "", MAX_TOKEN_LEN - 1);
    t.value[MAX_TOKEN_LEN - 1] = '\0';
    return t;
}

static TokenType keyword_lookup(const char *word) {
    for (int i = 0; KEYWORDS[i].word != NULL; i++) {
        if (strcmp(KEYWORDS[i].word, word) == 0)
            return KEYWORDS[i].type;
    }
    return TOK_IDENT;
}

/* ============================================================
 * LEXER INIT
 * ============================================================ */
void lexer_init(HolexaCompiler *c) {
    c->pos  = 0;
    c->line = 1;
    c->col  = 1;
    /* Prime the lookahead */
    c->current = lexer_next(c);
    c->peek    = lexer_next(c);
}

/* ============================================================
 * MAIN TOKENIZER
 * ============================================================ */
Token lexer_next(HolexaCompiler *c) {
    skip_whitespace_and_comments(c);

    if (c->pos >= (int)c->source_len)
        return make_token(c, TOK_EOF, "");

    int start_line = c->line;
    int start_col  = c->col;
    char ch = current_char(c);

    /* ---- Newline ---- */
    if (ch == '\n') {
        advance(c);
        Token t = make_token(c, TOK_NEWLINE, "\\n");
        t.line = start_line; t.col = start_col;
        return t;
    }

    /* ---- Integer / Float literals ---- */
    if (isdigit(ch)) {
        char buf[MAX_TOKEN_LEN];
        int  len = 0;
        bool is_float = false;

        /* Hex literal */
        if (ch == '0' && (peek_char(c) == 'x' || peek_char(c) == 'X')) {
            buf[len++] = advance(c); /* 0 */
            buf[len++] = advance(c); /* x */
            while (isxdigit(current_char(c)) || current_char(c) == '_') {
                if (current_char(c) != '_') buf[len++] = current_char(c);
                advance(c);
            }
            buf[len] = '\0';
            Token t = make_token(c, TOK_INT_LIT, buf);
            t.line = start_line; t.col = start_col;
            return t;
        }

        while (isdigit(current_char(c)) || current_char(c) == '_') {
            if (current_char(c) != '_') buf[len++] = current_char(c);
            advance(c);
        }
        if (current_char(c) == '.' && peek_char(c) != '.') {
            is_float = true;
            buf[len++] = advance(c);
            while (isdigit(current_char(c))) buf[len++] = advance(c);
        }
        if (current_char(c) == 'e' || current_char(c) == 'E') {
            is_float = true;
            buf[len++] = advance(c);
            if (current_char(c) == '+' || current_char(c) == '-')
                buf[len++] = advance(c);
            while (isdigit(current_char(c))) buf[len++] = advance(c);
        }
        buf[len] = '\0';
        Token t = make_token(c, is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, buf);
        t.line = start_line; t.col = start_col;
        return t;
    }

    /* ---- String literals ---- */
    if (ch == '"') {
        advance(c); /* skip opening " */
        char buf[MAX_STRING_LEN];
        int  len = 0;
        while (c->pos < (int)c->source_len && current_char(c) != '"') {
            if (current_char(c) == '\\') {
                advance(c);
                char esc = advance(c);
                switch (esc) {
                    case 'n':  buf[len++] = '\n'; break;
                    case 't':  buf[len++] = '\t'; break;
                    case 'r':  buf[len++] = '\r'; break;
                    case '"':  buf[len++] = '"';  break;
                    case '\\': buf[len++] = '\\'; break;
                    case '0':  buf[len++] = '\0'; break;
                    default:   buf[len++] = '\\'; buf[len++] = esc; break;
                }
            } else {
                buf[len++] = advance(c);
            }
            if (len >= MAX_STRING_LEN - 2) break;
        }
        if (current_char(c) == '"') advance(c); /* skip closing " */
        buf[len] = '\0';
        Token t = make_token(c, TOK_STRING_LIT, buf);
        t.line = start_line; t.col = start_col;
        return t;
    }

    /* ---- Char literal ---- */
    if (ch == '\'') {
        advance(c);
        char buf[8] = {0};
        if (current_char(c) == '\\') {
            advance(c);
            char esc = advance(c);
            switch(esc) {
                case 'n': buf[0] = '\n'; break;
                case 't': buf[0] = '\t'; break;
                default:  buf[0] = esc;  break;
            }
        } else {
            buf[0] = advance(c);
        }
        if (current_char(c) == '\'') advance(c);
        Token t = make_token(c, TOK_INT_LIT, buf); /* char as int */
        t.line = start_line; t.col = start_col;
        return t;
    }

    /* ---- Identifiers and keywords ---- */
    if (isalpha(ch) || ch == '_') {
        char buf[MAX_IDENTIFIER];
        int  len = 0;
        while (isalnum(current_char(c)) || current_char(c) == '_') {
            buf[len++] = advance(c);
            if (len >= MAX_IDENTIFIER - 1) break;
        }
        buf[len] = '\0';
        TokenType kw = keyword_lookup(buf);
        Token t = make_token(c, kw, buf);
        t.line = start_line; t.col = start_col;
        return t;
    }

    /* ---- Operators and punctuation ---- */
    advance(c); /* consume current char */

    #define MAKE(tt) do { Token t = make_token(c, tt, ""); t.line=start_line; t.col=start_col; return t; } while(0)
    #define MATCH(next, tt1, tt2) do { \
        if (current_char(c) == (next)) { advance(c); MAKE(tt1); } else { MAKE(tt2); } \
    } while(0)

    switch (ch) {
        case '+': MATCH('=', TOK_PLUS_EQ,    TOK_PLUS);
        case '-':
            if (current_char(c) == '>') { advance(c); MAKE(TOK_ARROW); }
            MATCH('=', TOK_MINUS_EQ, TOK_MINUS);
        case '*':
            if (current_char(c) == '*') { advance(c); MAKE(TOK_POWER); }
            MATCH('=', TOK_STAR_EQ,  TOK_STAR);
        case '/': MATCH('=', TOK_SLASH_EQ,   TOK_SLASH);
        case '%': MATCH('=', TOK_PERCENT_EQ, TOK_PERCENT);
        case '=':
            if (current_char(c) == '=') { advance(c); MAKE(TOK_EQEQ); }
            if (current_char(c) == '>') { advance(c); MAKE(TOK_FAT_ARROW); }
            MAKE(TOK_EQ);
        case '!': MATCH('=', TOK_NEQ,    TOK_NOT);
        case '<':
            if (current_char(c) == '<') { advance(c); MAKE(TOK_LSHIFT); }
            MATCH('=', TOK_LTE, TOK_LT);
        case '>':
            if (current_char(c) == '>') { advance(c); MAKE(TOK_RSHIFT); }
            MATCH('=', TOK_GTE, TOK_GT);
        case '&': MATCH('&', TOK_AND,    TOK_BAND);
        case '|': MATCH('|', TOK_OR,     TOK_BOR);
        case '^': MAKE(TOK_BXOR);
        case '~': MAKE(TOK_BNOT);
        case '.':
            if (current_char(c) == '.') {
                advance(c);
                if (current_char(c) == '=') { advance(c); MAKE(TOK_DOTDOTEQ); }
                MAKE(TOK_DOTDOT);
            }
            MAKE(TOK_DOT);
        case ':':
            if (current_char(c) == ':') { advance(c); MAKE(TOK_DOUBLECOLON); }
            MAKE(TOK_COLON);
        case '(': MAKE(TOK_LPAREN);
        case ')': MAKE(TOK_RPAREN);
        case '{': MAKE(TOK_LBRACE);
        case '}': MAKE(TOK_RBRACE);
        case '[': MAKE(TOK_LBRACKET);
        case ']': MAKE(TOK_RBRACKET);
        case ',': MAKE(TOK_COMMA);
        case ';': MAKE(TOK_SEMICOLON);
        case '?': MAKE(TOK_QUESTION);
        case '@': MAKE(TOK_AT);
        case '#': MAKE(TOK_HASH);
        default: {
            char buf[4] = {ch, '\0'};
            error_add(c, ERR_LEXER, 100, start_line, start_col,
                      "Unexpected character '%c' (0x%02x)", ch, (unsigned char)ch);
            Token t = make_token(c, TOK_ERROR, buf);
            t.line = start_line; t.col = start_col;
            return t;
        }
    }
    #undef MAKE
    #undef MATCH
}

/* ============================================================
 * TOKEN NAME (for error messages)
 * ============================================================ */
const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_INT_LIT:    return "integer";
        case TOK_FLOAT_LIT:  return "float";
        case TOK_STRING_LIT: return "string";
        case TOK_BOOL_LIT:   return "bool";
        case TOK_IDENT:      return "identifier";
        case TOK_LET:        return "'let'";
        case TOK_MUT:        return "'mut'";
        case TOK_FN:         return "'fn'";
        case TOK_RETURN:     return "'return'";
        case TOK_IF:         return "'if'";
        case TOK_ELSE:       return "'else'";
        case TOK_WHILE:      return "'while'";
        case TOK_FOR:        return "'for'";
        case TOK_IN:         return "'in'";
        case TOK_LOOP:       return "'loop'";
        case TOK_BREAK:      return "'break'";
        case TOK_CONTINUE:   return "'continue'";
        case TOK_STRUCT:     return "'struct'";
        case TOK_IMPL:       return "'impl'";
        case TOK_TRAIT:      return "'trait'";
        case TOK_IMPORT:     return "'import'";
        case TOK_USE:        return "'use'";
        case TOK_PUB:        return "'pub'";
        case TOK_MATCH:      return "'match'";
        case TOK_ENUM:       return "'enum'";
        case TOK_ASYNC:      return "'async'";
        case TOK_AWAIT:      return "'await'";
        case TOK_TRUE:       return "'true'";
        case TOK_FALSE:      return "'false'";
        case TOK_NONE:       return "'none'";
        case TOK_INT:        return "'int'";
        case TOK_FLOAT:      return "'float'";
        case TOK_STRING:     return "'String'";
        case TOK_BOOL:       return "'bool'";
        case TOK_VOID:       return "'void'";
        case TOK_PLUS:       return "'+'";
        case TOK_MINUS:      return "'-'";
        case TOK_STAR:       return "'*'";
        case TOK_SLASH:      return "'/'";
        case TOK_PERCENT:    return "'%'";
        case TOK_EQ:         return "'='";
        case TOK_EQEQ:       return "'=='";
        case TOK_NEQ:        return "'!='";
        case TOK_LT:         return "'<'";
        case TOK_GT:         return "'>'";
        case TOK_LTE:        return "'<='";
        case TOK_GTE:        return "'>='";
        case TOK_AND:        return "'&&'";
        case TOK_OR:         return "'||'";
        case TOK_NOT:        return "'!'";
        case TOK_ARROW:      return "'->'";
        case TOK_DOTDOT:     return "'..'";
        case TOK_LPAREN:     return "'('";
        case TOK_RPAREN:     return "')'";
        case TOK_LBRACE:     return "'{'";
        case TOK_RBRACE:     return "'}'";
        case TOK_LBRACKET:   return "'['";
        case TOK_RBRACKET:   return "']'";
        case TOK_COMMA:      return "','";
        case TOK_SEMICOLON:  return "';'";
        case TOK_COLON:      return "':'";
        case TOK_DOT:        return "'.'";
        case TOK_EOF:        return "end of file";
        case TOK_NEWLINE:    return "newline";
        default:             return "unknown";
    }
}
