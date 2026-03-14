/*
 * HOLEXA Programming Language Compiler
 * Version: 1.0.0
 * License: MIT
 * GitHub: https://github.com/HOLEXA-LANG/holexa-lang
 *
 * Compiler pipeline:
 *   Source (.hlx) -> Lexer -> Parser -> AST -> Semantic Analysis -> C Code -> GCC -> Binary
 */

#ifndef HOLEXA_H
#define HOLEXA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>

/* ============================================================
 * VERSION
 * ============================================================ */
#define HOLEXA_VERSION_MAJOR 1
#define HOLEXA_VERSION_MINOR 0
#define HOLEXA_VERSION_PATCH 0
#define HOLEXA_VERSION "1.0.0"

/* ============================================================
 * LIMITS
 * ============================================================ */
#define MAX_TOKEN_LEN     512
#define MAX_IDENTIFIER    256
#define MAX_STRING_LEN    4096
#define MAX_ERRORS        256
#define MAX_PARAMS        64
#define MAX_ARGS          64
#define MAX_STMTS         1024
#define MAX_MEMBERS       64
#define MAX_SCOPES        128

/* ============================================================
 * TOKEN TYPES
 * ============================================================ */
typedef enum {
    /* Literals */
    TOK_INT_LIT,
    TOK_FLOAT_LIT,
    TOK_STRING_LIT,
    TOK_BOOL_LIT,
    TOK_NULL_LIT,

    /* Identifiers */
    TOK_IDENT,

    /* Keywords */
    TOK_LET,
    TOK_MUT,
    TOK_FN,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
    TOK_LOOP,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_STRUCT,
    TOK_IMPL,
    TOK_TRAIT,
    TOK_IMPORT,
    TOK_USE,
    TOK_PUB,
    TOK_MOD,
    TOK_MATCH,
    TOK_ENUM,
    TOK_TYPE,
    TOK_ASYNC,
    TOK_AWAIT,
    TOK_SPAWN,
    TOK_EXTERN,
    TOK_CONST,
    TOK_STATIC,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NONE,
    TOK_SELF,
    TOK_NEW,
    TOK_DELETE,

    /* Types */
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_BOOL,
    TOK_VOID,
    TOK_BYTE,
    TOK_CHAR,
    TOK_I8, TOK_I16, TOK_I32, TOK_I64,
    TOK_U8, TOK_U16, TOK_U32, TOK_U64,
    TOK_F32, TOK_F64,

    /* Operators */
    TOK_PLUS,        /* + */
    TOK_MINUS,       /* - */
    TOK_STAR,        /* * */
    TOK_SLASH,       /* / */
    TOK_PERCENT,     /* % */
    TOK_POWER,       /* ** */
    TOK_EQ,          /* = */
    TOK_PLUS_EQ,     /* += */
    TOK_MINUS_EQ,    /* -= */
    TOK_STAR_EQ,     /* *= */
    TOK_SLASH_EQ,    /* /= */
    TOK_PERCENT_EQ,  /* %= */
    TOK_EQEQ,        /* == */
    TOK_NEQ,         /* != */
    TOK_LT,          /* < */
    TOK_GT,          /* > */
    TOK_LTE,         /* <= */
    TOK_GTE,         /* >= */
    TOK_AND,         /* && */
    TOK_OR,          /* || */
    TOK_NOT,         /* ! */
    TOK_BAND,        /* & */
    TOK_BOR,         /* | */
    TOK_BXOR,        /* ^ */
    TOK_BNOT,        /* ~ */
    TOK_LSHIFT,      /* << */
    TOK_RSHIFT,      /* >> */
    TOK_ARROW,       /* -> */
    TOK_FAT_ARROW,   /* => */
    TOK_DOTDOT,      /* .. */
    TOK_DOTDOTEQ,    /* ..= */
    TOK_QUESTION,    /* ? */
    TOK_AT,          /* @ */
    TOK_HASH,        /* # */

    /* Delimiters */
    TOK_LPAREN,      /* ( */
    TOK_RPAREN,      /* ) */
    TOK_LBRACE,      /* { */
    TOK_RBRACE,      /* } */
    TOK_LBRACKET,    /* [ */
    TOK_RBRACKET,    /* ] */
    TOK_COMMA,       /* , */
    TOK_SEMICOLON,   /* ; */
    TOK_COLON,       /* : */
    TOK_DOUBLECOLON, /* :: */
    TOK_DOT,         /* . */

    /* Special */
    TOK_NEWLINE,
    TOK_EOF,
    TOK_ERROR,
    TOK_COUNT
} TokenType;

/* ============================================================
 * TOKEN
 * ============================================================ */
typedef struct {
    TokenType   type;
    char        value[MAX_TOKEN_LEN];
    int         line;
    int         col;
    const char *filename;
} Token;

/* ============================================================
 * AST NODE TYPES
 * ============================================================ */
typedef enum {
    /* Program */
    AST_PROGRAM,

    /* Declarations */
    AST_VAR_DECL,
    AST_CONST_DECL,
    AST_FN_DECL,
    AST_STRUCT_DECL,
    AST_ENUM_DECL,
    AST_TRAIT_DECL,
    AST_IMPL_BLOCK,
    AST_MOD_DECL,
    AST_IMPORT,
    AST_USE,
    AST_EXTERN_DECL,

    /* Statements */
    AST_BLOCK,
    AST_RETURN,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_LOOP,
    AST_BREAK,
    AST_CONTINUE,
    AST_MATCH,
    AST_EXPR_STMT,
    AST_ASSIGN,

    /* Expressions */
    AST_BINARY,
    AST_UNARY,
    AST_CALL,
    AST_INDEX,
    AST_FIELD,
    AST_CAST,
    AST_RANGE,
    AST_AWAIT,
    AST_SPAWN,

    /* Literals */
    AST_INT_LIT,
    AST_FLOAT_LIT,
    AST_STRING_LIT,
    AST_BOOL_LIT,
    AST_NONE_LIT,
    AST_ARRAY_LIT,
    AST_STRUCT_LIT,
    AST_TUPLE_LIT,

    /* Types */
    AST_TYPE_NAME,
    AST_TYPE_ARRAY,
    AST_TYPE_MAP,
    AST_TYPE_FN,
    AST_TYPE_OPT,
    AST_TYPE_REF,
    AST_TYPE_TUPLE,
    AST_GENERIC,

    /* Misc */
    AST_IDENT,
    AST_PARAM,
    AST_MATCH_ARM,
    AST_CLOSURE,
    AST_MACRO_CALL,

    AST_COUNT
} ASTNodeType;

/* ============================================================
 * HOLEXA TYPE SYSTEM
 * ============================================================ */
typedef enum {
    HTYPE_UNKNOWN,
    HTYPE_VOID,
    HTYPE_BOOL,
    HTYPE_INT,       /* default integer = i64 */
    HTYPE_FLOAT,     /* default float   = f64 */
    HTYPE_I8, HTYPE_I16, HTYPE_I32, HTYPE_I64,
    HTYPE_U8, HTYPE_U16, HTYPE_U32, HTYPE_U64,
    HTYPE_F32, HTYPE_F64,
    HTYPE_CHAR,
    HTYPE_BYTE,
    HTYPE_STRING,
    HTYPE_ARRAY,
    HTYPE_MAP,
    HTYPE_TUPLE,
    HTYPE_STRUCT,
    HTYPE_ENUM,
    HTYPE_TRAIT,
    HTYPE_FN,
    HTYPE_OPTIONAL,
    HTYPE_REF,
    HTYPE_NONE,
    HTYPE_ASYNC,
} HolexaType;

/* Forward declarations */
struct ASTNode;
struct TypeInfo;

typedef struct TypeInfo {
    HolexaType   base;
    char         name[MAX_IDENTIFIER];
    struct TypeInfo *inner;   /* for Array<T>, Optional<T>, Ref<T> */
    struct TypeInfo *key;     /* for Map<K, V> */
    bool         is_mut;
    bool         is_async;
    int          array_size;  /* -1 = dynamic */
} TypeInfo;

/* ============================================================
 * AST NODE
 * ============================================================ */
typedef struct ASTNode {
    ASTNodeType  type;
    int          line;
    int          col;
    TypeInfo    *resolved_type;  /* filled by semantic analysis */

    union {
        /* INT literal */
        struct { long long value; } int_lit;

        /* FLOAT literal */
        struct { double value; } float_lit;

        /* STRING literal */
        struct { char value[MAX_STRING_LEN]; } string_lit;

        /* BOOL literal */
        struct { bool value; } bool_lit;

        /* IDENTIFIER */
        struct { char name[MAX_IDENTIFIER]; } ident;

        /* VAR / CONST DECL: let x: Type = expr */
        struct {
            char             name[MAX_IDENTIFIER];
            struct ASTNode  *type_node;
            struct ASTNode  *init;
            bool             is_mut;
            bool             is_pub;
        } var_decl;

        /* FUNCTION DECL */
        struct {
            char             name[MAX_IDENTIFIER];
            struct ASTNode  *params[MAX_PARAMS];
            int              param_count;
            struct ASTNode  *return_type;
            struct ASTNode  *body;
            bool             is_pub;
            bool             is_async;
            bool             is_extern;
        } fn_decl;

        /* PARAM */
        struct {
            char             name[MAX_IDENTIFIER];
            struct ASTNode  *type_node;
            struct ASTNode  *default_val;
            bool             is_mut;
        } param;

        /* STRUCT DECL */
        struct {
            char             name[MAX_IDENTIFIER];
            struct ASTNode  *fields[MAX_MEMBERS];
            int              field_count;
            bool             is_pub;
        } struct_decl;

        /* ENUM DECL */
        struct {
            char             name[MAX_IDENTIFIER];
            struct ASTNode  *variants[MAX_MEMBERS];
            int              variant_count;
            bool             is_pub;
        } enum_decl;

        /* IMPL BLOCK */
        struct {
            char             target[MAX_IDENTIFIER];
            char             trait_name[MAX_IDENTIFIER]; /* empty = no trait */
            struct ASTNode  *methods[MAX_MEMBERS];
            int              method_count;
        } impl_block;

        /* IMPORT / USE */
        struct {
            char             path[MAX_STRING_LEN];
            char             alias[MAX_IDENTIFIER];
        } import;

        /* BLOCK */
        struct {
            struct ASTNode  *stmts[MAX_STMTS];
            int              count;
        } block;

        /* RETURN */
        struct {
            struct ASTNode  *value;
        } ret;

        /* IF */
        struct {
            struct ASTNode  *cond;
            struct ASTNode  *then_block;
            struct ASTNode  *else_block;  /* NULL or another IF node */
        } if_stmt;

        /* WHILE */
        struct {
            struct ASTNode  *cond;
            struct ASTNode  *body;
        } while_stmt;

        /* FOR */
        struct {
            char             var[MAX_IDENTIFIER];
            struct ASTNode  *iter;
            struct ASTNode  *body;
        } for_stmt;

        /* LOOP (infinite) */
        struct {
            struct ASTNode  *body;
        } loop_stmt;

        /* BINARY EXPR */
        struct {
            TokenType        op;
            struct ASTNode  *left;
            struct ASTNode  *right;
        } binary;

        /* UNARY EXPR */
        struct {
            TokenType        op;
            struct ASTNode  *operand;
        } unary;

        /* CALL EXPR */
        struct {
            struct ASTNode  *callee;
            struct ASTNode  *args[MAX_ARGS];
            int              arg_count;
        } call;

        /* ASSIGN */
        struct {
            struct ASTNode  *target;
            TokenType        op;
            struct ASTNode  *value;
        } assign;

        /* INDEX */
        struct {
            struct ASTNode  *object;
            struct ASTNode  *index;
        } index_expr;

        /* FIELD ACCESS */
        struct {
            struct ASTNode  *object;
            char             field[MAX_IDENTIFIER];
        } field;

        /* RANGE */
        struct {
            struct ASTNode  *start;
            struct ASTNode  *end;
            bool             inclusive;
        } range;

        /* TYPE NAME */
        struct {
            char             name[MAX_IDENTIFIER];
            struct ASTNode  *params[MAX_PARAMS]; /* generics */
            int              param_count;
        } type_name;

        /* ARRAY LITERAL */
        struct {
            struct ASTNode  *elems[MAX_ARGS];
            int              count;
        } array_lit;

        /* MATCH */
        struct {
            struct ASTNode  *value;
            struct ASTNode  *arms[MAX_STMTS];
            int              arm_count;
        } match_stmt;

        /* MATCH ARM */
        struct {
            struct ASTNode  *pattern;
            struct ASTNode  *guard;
            struct ASTNode  *body;
        } match_arm;

        /* AWAIT */
        struct {
            struct ASTNode  *expr;
        } await_expr;

        /* CAST */
        struct {
            struct ASTNode  *expr;
            struct ASTNode  *type_node;
        } cast_expr;

    } as;
} ASTNode;

/* ============================================================
 * SYMBOL TABLE
 * ============================================================ */
typedef struct Symbol {
    char         name[MAX_IDENTIFIER];
    TypeInfo    *type;
    bool         is_mut;
    bool         is_fn;
    bool         is_type;
    int          scope_depth;
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    Symbol      *symbols;
    struct Scope *parent;
    int          depth;
} Scope;

/* ============================================================
 * ERROR SYSTEM
 * ============================================================ */
typedef enum {
    ERR_NONE = 0,
    ERR_LEXER,
    ERR_PARSER,
    ERR_SEMANTIC,
    ERR_TYPE,
    ERR_UNDEFINED,
    ERR_REDEFINED,
    ERR_RETURN,
    ERR_CODEGEN,
    ERR_IO,
} ErrorKind;

typedef struct {
    ErrorKind    kind;
    int          code;       /* HLX error code e.g. 203 */
    char         message[512];
    char         filename[256];
    int          line;
    int          col;
} HolexaError;

typedef struct {
    HolexaError  errors[MAX_ERRORS];
    int          count;
    bool         has_error;
} ErrorList;

/* ============================================================
 * COMPILER CONTEXT
 * ============================================================ */
typedef struct {
    const char  *source;
    size_t       source_len;
    const char  *filename;

    /* Lexer state */
    int          pos;
    int          line;
    int          col;
    Token        current;
    Token        peek;

    /* AST root */
    ASTNode     *ast;

    /* Scope / symbol table */
    Scope       *scope;
    int          scope_depth;

    /* Code generation output */
    FILE        *out;
    int          tmp_var_count;
    int          label_count;

    /* Errors */
    ErrorList    errors;

    /* Flags */
    bool         verbose;
    bool         debug_ast;
    bool         debug_tokens;
    bool         optimize;
} HolexaCompiler;

/* ============================================================
 * FUNCTION PROTOTYPES
 * ============================================================ */

/* compiler.c */
HolexaCompiler *compiler_new(const char *filename, const char *source, bool verbose);
void            compiler_free(HolexaCompiler *c);
bool            compiler_compile(HolexaCompiler *c, const char *out_c_file);

/* lexer.c */
void            lexer_init(HolexaCompiler *c);
Token           lexer_next(HolexaCompiler *c);
Token           lexer_peek(HolexaCompiler *c);
const char     *token_type_name(TokenType t);

/* parser.c */
ASTNode        *parser_parse(HolexaCompiler *c);
ASTNode        *ast_node_new(HolexaCompiler *c, ASTNodeType type, int line, int col);

/* semantic.c */
bool            semantic_analyze(HolexaCompiler *c, ASTNode *ast);

/* codegen.c */
bool            codegen_emit(HolexaCompiler *c, ASTNode *ast, FILE *out);

/* error.c */
void            error_add(HolexaCompiler *c, ErrorKind kind, int code,
                          int line, int col, const char *fmt, ...);
void            error_print_all(HolexaCompiler *c);
bool            error_has(HolexaCompiler *c);

/* util.c */
char           *read_file(const char *path);
char           *str_dup(const char *s);
char           *str_concat(const char *a, const char *b);
void            str_escape(const char *in, char *out, size_t out_size);
TypeInfo       *type_new(HolexaType base);
TypeInfo       *type_named(const char *name);
bool            type_equal(TypeInfo *a, TypeInfo *b);
const char     *type_to_c(TypeInfo *t);
const char     *htype_name(HolexaType t);

/* scope.c */
Scope          *scope_new(Scope *parent);
void            scope_free(Scope *s);
Symbol         *scope_define(HolexaCompiler *c, const char *name, TypeInfo *type, bool is_mut);
Symbol         *scope_lookup(HolexaCompiler *c, const char *name);
void            scope_push(HolexaCompiler *c);
void            scope_pop(HolexaCompiler *c);

/* ast_print.c (debug) */
void            ast_print(ASTNode *node, int depth);

#endif /* HOLEXA_H */
