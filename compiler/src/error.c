/*
 * HOLEXA Error System & Utilities
 */

#include "holexa.h"

/* ============================================================
 * ERROR SYSTEM
 * ============================================================ */
void error_add(HolexaCompiler *c, ErrorKind kind, int code,
               int line, int col, const char *fmt, ...) {
    if (c->errors.count >= MAX_ERRORS) return;

    HolexaError *e = &c->errors.errors[c->errors.count++];
    e->kind = kind;
    e->code = code;
    e->line = line;
    e->col  = col;
    if (c->filename)
        strncpy(e->filename, c->filename, 255);

    va_list args;
    va_start(args, fmt);
    vsnprintf(e->message, sizeof(e->message), fmt, args);
    va_end(args);

    c->errors.has_error = true;
}

static const char *error_kind_name(ErrorKind k) {
    switch (k) {
        case ERR_LEXER:    return "Lexer Error";
        case ERR_PARSER:   return "Parse Error";
        case ERR_SEMANTIC: return "Semantic Error";
        case ERR_TYPE:     return "Type Error";
        case ERR_UNDEFINED:return "Undefined Symbol";
        case ERR_REDEFINED:return "Redefinition Error";
        case ERR_RETURN:   return "Return Error";
        case ERR_CODEGEN:  return "Code Generation Error";
        case ERR_IO:       return "I/O Error";
        default:           return "Error";
    }
}

void error_print_all(HolexaCompiler *c) {
    for (int i = 0; i < c->errors.count; i++) {
        HolexaError *e = &c->errors.errors[i];
        fprintf(stderr,
            "\n\033[1;31m[Error HLX%03d]\033[0m %s\n"
            "  \033[1;36m%s\033[0m\n"
            "  \033[90m--> %s:%d:%d\033[0m\n",
            e->code,
            error_kind_name(e->kind),
            e->message,
            e->filename[0] ? e->filename : "<unknown>",
            e->line, e->col);
    }
}

bool error_has(HolexaCompiler *c) {
    return c->errors.has_error;
}

/* ============================================================
 * FILE UTILITIES
 * ============================================================ */
char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t read = fread(buf, 1, size, f);
    fclose(f);
    buf[read] = '\0';
    return buf;
}

char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *r = malloc(len + 1);
    memcpy(r, s, len + 1);
    return r;
}

char *str_concat(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *r = malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb + 1);
    return r;
}

void str_escape(const char *in, char *out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; in[i] && j < out_size - 2; i++) {
        if (in[i] == '"' || in[i] == '\\') out[j++] = '\\';
        out[j++] = in[i];
    }
    out[j] = '\0';
}

/* ============================================================
 * AST PRINTER (debug)
 * ============================================================ */
static const char *ast_type_name(ASTNodeType t) {
    switch (t) {
        case AST_PROGRAM:     return "Program";
        case AST_VAR_DECL:    return "VarDecl";
        case AST_CONST_DECL:  return "ConstDecl";
        case AST_FN_DECL:     return "FnDecl";
        case AST_STRUCT_DECL: return "StructDecl";
        case AST_ENUM_DECL:   return "EnumDecl";
        case AST_IMPL_BLOCK:  return "ImplBlock";
        case AST_BLOCK:       return "Block";
        case AST_RETURN:      return "Return";
        case AST_IF:          return "If";
        case AST_WHILE:       return "While";
        case AST_FOR:         return "For";
        case AST_LOOP:        return "Loop";
        case AST_BREAK:       return "Break";
        case AST_CONTINUE:    return "Continue";
        case AST_MATCH:       return "Match";
        case AST_EXPR_STMT:   return "ExprStmt";
        case AST_ASSIGN:      return "Assign";
        case AST_BINARY:      return "Binary";
        case AST_UNARY:       return "Unary";
        case AST_CALL:        return "Call";
        case AST_INDEX:       return "Index";
        case AST_FIELD:       return "Field";
        case AST_INT_LIT:     return "IntLit";
        case AST_FLOAT_LIT:   return "FloatLit";
        case AST_STRING_LIT:  return "StringLit";
        case AST_BOOL_LIT:    return "BoolLit";
        case AST_NONE_LIT:    return "NoneLit";
        case AST_ARRAY_LIT:   return "ArrayLit";
        case AST_IDENT:       return "Ident";
        case AST_RANGE:       return "Range";
        case AST_IMPORT:      return "Import";
        case AST_PARAM:       return "Param";
        default:              return "Unknown";
    }
}

void ast_print(ASTNode *node, int depth) {
    if (!node) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("[%s]", ast_type_name(node->type));

    switch (node->type) {
        case AST_INT_LIT:    printf(" %lld", node->as.int_lit.value); break;
        case AST_FLOAT_LIT:  printf(" %g", node->as.float_lit.value); break;
        case AST_STRING_LIT: printf(" \"%s\"", node->as.string_lit.value); break;
        case AST_BOOL_LIT:   printf(" %s", node->as.bool_lit.value?"true":"false"); break;
        case AST_IDENT:      printf(" %s", node->as.ident.name); break;
        case AST_VAR_DECL:   printf(" %s%s", node->as.var_decl.is_mut?"mut ":"", node->as.var_decl.name); break;
        case AST_FN_DECL:    printf(" %s", node->as.fn_decl.name); break;
        case AST_STRUCT_DECL:printf(" %s", node->as.struct_decl.name); break;
        case AST_IMPORT:     printf(" \"%s\"", node->as.import.path); break;
        default: break;
    }
    printf(" (line %d)\n", node->line);
}
