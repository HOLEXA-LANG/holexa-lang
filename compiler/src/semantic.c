/*
 * HOLEXA Semantic Analyzer
 * Type checking, scope analysis, symbol resolution
 */

#include "holexa.h"

/* ============================================================
 * TYPE UTILITIES
 * ============================================================ */

TypeInfo *type_new(HolexaType base) {
    TypeInfo *t = calloc(1, sizeof(TypeInfo));
    t->base = base;
    strncpy(t->name, htype_name(base), MAX_IDENTIFIER-1);
    return t;
}

TypeInfo *type_named(const char *name) {
    TypeInfo *t = calloc(1, sizeof(TypeInfo));
    t->base = HTYPE_STRUCT; /* assume struct/named type */
    strncpy(t->name, name, MAX_IDENTIFIER-1);
    return t;
}

bool type_equal(TypeInfo *a, TypeInfo *b) {
    if (!a || !b) return false;
    if (a->base != b->base) return false;
    if (a->base == HTYPE_STRUCT || a->base == HTYPE_ENUM)
        return strcmp(a->name, b->name) == 0;
    return true;
}

const char *htype_name(HolexaType t) {
    switch (t) {
        case HTYPE_VOID:    return "void";
        case HTYPE_BOOL:    return "bool";
        case HTYPE_INT:     return "int";
        case HTYPE_FLOAT:   return "float";
        case HTYPE_I8:      return "i8";
        case HTYPE_I16:     return "i16";
        case HTYPE_I32:     return "i32";
        case HTYPE_I64:     return "i64";
        case HTYPE_U8:      return "u8";
        case HTYPE_U16:     return "u16";
        case HTYPE_U32:     return "u32";
        case HTYPE_U64:     return "u64";
        case HTYPE_F32:     return "f32";
        case HTYPE_F64:     return "f64";
        case HTYPE_CHAR:    return "char";
        case HTYPE_BYTE:    return "byte";
        case HTYPE_STRING:  return "String";
        case HTYPE_ARRAY:   return "Array";
        case HTYPE_MAP:     return "Map";
        case HTYPE_STRUCT:  return "struct";
        case HTYPE_ENUM:    return "enum";
        case HTYPE_FN:      return "fn";
        case HTYPE_NONE:    return "none";
        default:            return "unknown";
    }
}

const char *type_to_c(TypeInfo *t) {
    if (!t) return "void";
    switch (t->base) {
        case HTYPE_VOID:   return "void";
        case HTYPE_BOOL:   return "int";        /* C _Bool */
        case HTYPE_INT:    return "long long";
        case HTYPE_I8:     return "int8_t";
        case HTYPE_I16:    return "int16_t";
        case HTYPE_I32:    return "int32_t";
        case HTYPE_I64:    return "int64_t";
        case HTYPE_U8:     return "uint8_t";
        case HTYPE_U16:    return "uint16_t";
        case HTYPE_U32:    return "uint32_t";
        case HTYPE_U64:    return "uint64_t";
        case HTYPE_F32:    return "float";
        case HTYPE_FLOAT:
        case HTYPE_F64:    return "double";
        case HTYPE_CHAR:   return "char";
        case HTYPE_BYTE:   return "unsigned char";
        case HTYPE_STRING: return "char*";
        case HTYPE_ARRAY:  return "hlx_array*";
        case HTYPE_NONE:   return "void*";
        case HTYPE_STRUCT: return t->name; /* use struct name */
        default:           return "void*";
    }
}

/* ============================================================
 * SCOPE MANAGEMENT
 * ============================================================ */

Scope *scope_new(Scope *parent) {
    Scope *s = calloc(1, sizeof(Scope));
    s->parent  = parent;
    s->depth   = parent ? parent->depth + 1 : 0;
    s->symbols = NULL;
    return s;
}

void scope_free(Scope *s) {
    Symbol *sym = s->symbols;
    while (sym) {
        Symbol *next = sym->next;
        free(sym);
        sym = next;
    }
    free(s);
}

void scope_push(HolexaCompiler *c) {
    c->scope = scope_new(c->scope);
    c->scope_depth++;
}

void scope_pop(HolexaCompiler *c) {
    if (!c->scope) return;
    Scope *old = c->scope;
    c->scope = old->parent;
    c->scope_depth--;
    scope_free(old);
}

Symbol *scope_define(HolexaCompiler *c, const char *name, TypeInfo *type, bool is_mut) {
    /* Check for redefinition in current scope only */
    Symbol *existing = c->scope ? c->scope->symbols : NULL;
    while (existing) {
        if (strcmp(existing->name, name) == 0) {
            error_add(c, ERR_REDEFINED, 301, 0, 0,
                      "Symbol '%s' is already defined in this scope", name);
            return existing;
        }
        existing = existing->next;
    }

    Symbol *sym = calloc(1, sizeof(Symbol));
    strncpy(sym->name, name, MAX_IDENTIFIER-1);
    sym->type        = type;
    sym->is_mut      = is_mut;
    sym->scope_depth = c->scope_depth;
    sym->next        = c->scope ? c->scope->symbols : NULL;
    if (c->scope) c->scope->symbols = sym;
    return sym;
}

Symbol *scope_lookup(HolexaCompiler *c, const char *name) {
    Scope *s = c->scope;
    while (s) {
        Symbol *sym = s->symbols;
        while (sym) {
            if (strcmp(sym->name, name) == 0) return sym;
            sym = sym->next;
        }
        s = s->parent;
    }
    return NULL;
}

/* ============================================================
 * TYPE RESOLUTION FROM AST TYPE NODE
 * ============================================================ */
static TypeInfo *resolve_type_node(HolexaCompiler *c, ASTNode *type_node) {
    if (!type_node) return type_new(HTYPE_VOID);

    const char *name = type_node->as.type_name.name;

    if (strcmp(name, "int")    == 0 || strcmp(name, "i64") == 0) return type_new(HTYPE_INT);
    if (strcmp(name, "i32")    == 0) return type_new(HTYPE_I32);
    if (strcmp(name, "i16")    == 0) return type_new(HTYPE_I16);
    if (strcmp(name, "i8")     == 0) return type_new(HTYPE_I8);
    if (strcmp(name, "u64")    == 0) return type_new(HTYPE_U64);
    if (strcmp(name, "u32")    == 0) return type_new(HTYPE_U32);
    if (strcmp(name, "u16")    == 0) return type_new(HTYPE_U16);
    if (strcmp(name, "u8")     == 0) return type_new(HTYPE_U8);
    if (strcmp(name, "float")  == 0 || strcmp(name, "f64") == 0) return type_new(HTYPE_FLOAT);
    if (strcmp(name, "f32")    == 0) return type_new(HTYPE_F32);
    if (strcmp(name, "bool")   == 0) return type_new(HTYPE_BOOL);
    if (strcmp(name, "String") == 0) return type_new(HTYPE_STRING);
    if (strcmp(name, "str")    == 0) return type_new(HTYPE_STRING);
    if (strcmp(name, "char")   == 0) return type_new(HTYPE_CHAR);
    if (strcmp(name, "byte")   == 0) return type_new(HTYPE_BYTE);
    if (strcmp(name, "void")   == 0) return type_new(HTYPE_VOID);
    if (strcmp(name, "none")   == 0) return type_new(HTYPE_NONE);
    if (strcmp(name, "Array")  == 0) {
        TypeInfo *t = type_new(HTYPE_ARRAY);
        if (type_node->as.type_name.param_count > 0)
            t->inner = resolve_type_node(c, type_node->as.type_name.params[0]);
        return t;
    }
    if (strcmp(name, "Optional") == 0) {
        TypeInfo *t = type_new(HTYPE_OPTIONAL);
        if (type_node->as.type_name.param_count > 0)
            t->inner = resolve_type_node(c, type_node->as.type_name.params[0]);
        return t;
    }

    /* Named struct / enum type */
    return type_named(name);
}

/* ============================================================
 * SEMANTIC ANALYSIS
 * ============================================================ */
static TypeInfo *analyze_expr(HolexaCompiler *c, ASTNode *node);
static void      analyze_stmt(HolexaCompiler *c, ASTNode *node);
static void      analyze_block(HolexaCompiler *c, ASTNode *block);

static TypeInfo *analyze_expr(HolexaCompiler *c, ASTNode *node) {
    if (!node) return type_new(HTYPE_VOID);

    switch (node->type) {
        case AST_INT_LIT:
            node->resolved_type = type_new(HTYPE_INT);
            return node->resolved_type;

        case AST_FLOAT_LIT:
            node->resolved_type = type_new(HTYPE_FLOAT);
            return node->resolved_type;

        case AST_STRING_LIT:
            node->resolved_type = type_new(HTYPE_STRING);
            return node->resolved_type;

        case AST_BOOL_LIT:
            node->resolved_type = type_new(HTYPE_BOOL);
            return node->resolved_type;

        case AST_NONE_LIT:
            node->resolved_type = type_new(HTYPE_NONE);
            return node->resolved_type;

        case AST_IDENT: {
            Symbol *sym = scope_lookup(c, node->as.ident.name);
            if (!sym) {
                /* Built-in functions are allowed */
                if (strcmp(node->as.ident.name, "print")   == 0 ||
                    strcmp(node->as.ident.name, "println") == 0 ||
                    strcmp(node->as.ident.name, "input")   == 0 ||
                    strcmp(node->as.ident.name, "len")     == 0 ||
                    strcmp(node->as.ident.name, "exit")    == 0 ||
                    strcmp(node->as.ident.name, "assert")  == 0 ||
                    strcmp(node->as.ident.name, "typeof")  == 0 ||
                    strcmp(node->as.ident.name, "to_string") == 0 ||
                    strcmp(node->as.ident.name, "to_int")  == 0 ||
                    strcmp(node->as.ident.name, "to_float") == 0) {
                    node->resolved_type = type_new(HTYPE_FN);
                    return node->resolved_type;
                }
                error_add(c, ERR_UNDEFINED, 302, node->line, node->col,
                          "Undefined symbol '%s'", node->as.ident.name);
                node->resolved_type = type_new(HTYPE_UNKNOWN);
                return node->resolved_type;
            }
            node->resolved_type = sym->type ? sym->type : type_new(HTYPE_UNKNOWN);
            return node->resolved_type;
        }

        case AST_BINARY: {
            TypeInfo *lt = analyze_expr(c, node->as.binary.left);
            TypeInfo *rt = analyze_expr(c, node->as.binary.right);
            TokenType op = node->as.binary.op;

            /* Comparison operators always return bool */
            if (op == TOK_EQEQ || op == TOK_NEQ || op == TOK_LT ||
                op == TOK_GT   || op == TOK_LTE  || op == TOK_GTE ||
                op == TOK_AND  || op == TOK_OR) {
                node->resolved_type = type_new(HTYPE_BOOL);
                return node->resolved_type;
            }

            /* String concatenation: String + anything = String */
            if (lt->base == HTYPE_STRING || rt->base == HTYPE_STRING) {
                node->resolved_type = type_new(HTYPE_STRING);
                return node->resolved_type;
            }

            /* Float promotion */
            if (lt->base == HTYPE_FLOAT || rt->base == HTYPE_FLOAT ||
                lt->base == HTYPE_F64   || rt->base == HTYPE_F64) {
                node->resolved_type = type_new(HTYPE_FLOAT);
                return node->resolved_type;
            }

            node->resolved_type = lt;
            return node->resolved_type;
        }

        case AST_UNARY: {
            TypeInfo *t = analyze_expr(c, node->as.unary.operand);
            if (node->as.unary.op == TOK_NOT)
                node->resolved_type = type_new(HTYPE_BOOL);
            else
                node->resolved_type = t;
            return node->resolved_type;
        }

        case AST_CALL: {
            /* Analyze callee and arguments */
            analyze_expr(c, node->as.call.callee);
            for (int i = 0; i < node->as.call.arg_count; i++)
                analyze_expr(c, node->as.call.args[i]);
            /* Return type inference: look up function symbol if direct call */
            if (node->as.call.callee && node->as.call.callee->type == AST_IDENT) {
                Symbol *sym = scope_lookup(c, node->as.call.callee->as.ident.name);
                if (sym && sym->type) {
                    node->resolved_type = sym->type;
                    return node->resolved_type;
                }
            }
            node->resolved_type = type_new(HTYPE_UNKNOWN);
            return node->resolved_type;
        }

        case AST_FIELD: {
            analyze_expr(c, node->as.field.object);
            node->resolved_type = type_new(HTYPE_UNKNOWN);
            return node->resolved_type;
        }

        case AST_INDEX: {
            TypeInfo *obj = analyze_expr(c, node->as.index_expr.object);
            analyze_expr(c, node->as.index_expr.index);
            if (obj->base == HTYPE_ARRAY && obj->inner)
                node->resolved_type = obj->inner;
            else if (obj->base == HTYPE_STRING)
                node->resolved_type = type_new(HTYPE_CHAR);
            else
                node->resolved_type = type_new(HTYPE_UNKNOWN);
            return node->resolved_type;
        }

        case AST_ARRAY_LIT: {
            TypeInfo *elem_type = type_new(HTYPE_UNKNOWN);
            for (int i = 0; i < node->as.array_lit.count; i++) {
                TypeInfo *et = analyze_expr(c, node->as.array_lit.elems[i]);
                if (i == 0) elem_type = et;
            }
            TypeInfo *at = type_new(HTYPE_ARRAY);
            at->inner = elem_type;
            node->resolved_type = at;
            return node->resolved_type;
        }

        case AST_ASSIGN: {
            analyze_expr(c, node->as.assign.target);
            TypeInfo *vt = analyze_expr(c, node->as.assign.value);
            node->resolved_type = vt;
            return node->resolved_type;
        }

        case AST_RANGE: {
            analyze_expr(c, node->as.range.start);
            analyze_expr(c, node->as.range.end);
            node->resolved_type = type_new(HTYPE_ARRAY); /* range is iterable */
            return node->resolved_type;
        }

        case AST_AWAIT: {
            TypeInfo *inner = analyze_expr(c, node->as.await_expr.expr);
            node->resolved_type = inner;
            return node->resolved_type;
        }

        default:
            node->resolved_type = type_new(HTYPE_UNKNOWN);
            return node->resolved_type;
    }
}

static void analyze_block(HolexaCompiler *c, ASTNode *block) {
    if (!block) return;
    scope_push(c);
    for (int i = 0; i < block->as.block.count; i++)
        analyze_stmt(c, block->as.block.stmts[i]);
    scope_pop(c);
}

static void analyze_stmt(HolexaCompiler *c, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_VAR_DECL:
        case AST_CONST_DECL: {
            TypeInfo *declared_type = NULL;
            if (node->as.var_decl.type_node)
                declared_type = resolve_type_node(c, node->as.var_decl.type_node);

            TypeInfo *init_type = NULL;
            if (node->as.var_decl.init)
                init_type = analyze_expr(c, node->as.var_decl.init);

            TypeInfo *final_type = declared_type ? declared_type : init_type;
            if (!final_type) final_type = type_new(HTYPE_UNKNOWN);

            /* Type mismatch check */
            if (declared_type && init_type &&
                declared_type->base != HTYPE_UNKNOWN &&
                init_type->base     != HTYPE_UNKNOWN &&
                !type_equal(declared_type, init_type)) {
                /* Allow int -> float coercion */
                if (!(declared_type->base == HTYPE_FLOAT && init_type->base == HTYPE_INT) &&
                    !(declared_type->base == HTYPE_F64   && init_type->base == HTYPE_INT)) {
                    error_add(c, ERR_TYPE, 203, node->line, node->col,
                              "Type mismatch: variable '%s' declared as '%s' but initialized with '%s'",
                              node->as.var_decl.name,
                              htype_name(declared_type->base),
                              htype_name(init_type->base));
                }
            }

            scope_define(c, node->as.var_decl.name, final_type,
                         node->as.var_decl.is_mut || node->type == AST_CONST_DECL);
            node->resolved_type = final_type;
            break;
        }

        case AST_FN_DECL: {
            TypeInfo *ret_type = type_new(HTYPE_VOID);
            if (node->as.fn_decl.return_type)
                ret_type = resolve_type_node(c, node->as.fn_decl.return_type);

            /* Register function in current scope */
            if (node->as.fn_decl.name[0])
                scope_define(c, node->as.fn_decl.name, ret_type, false);

            /* Analyze body in new scope with params */
            if (node->as.fn_decl.body) {
                scope_push(c);
                for (int i = 0; i < node->as.fn_decl.param_count; i++) {
                    ASTNode *p = node->as.fn_decl.params[i];
                    if (!p) continue;
                    TypeInfo *pt = p->as.param.type_node
                        ? resolve_type_node(c, p->as.param.type_node)
                        : type_new(HTYPE_UNKNOWN);
                    scope_define(c, p->as.param.name, pt, p->as.param.is_mut);
                }
                /* Analyze body statements without another scope_push */
                for (int i = 0; i < node->as.fn_decl.body->as.block.count; i++)
                    analyze_stmt(c, node->as.fn_decl.body->as.block.stmts[i]);
                scope_pop(c);
            }
            break;
        }

        case AST_STRUCT_DECL: {
            TypeInfo *st = type_named(node->as.struct_decl.name);
            st->base = HTYPE_STRUCT;
            scope_define(c, node->as.struct_decl.name, st, false);
            break;
        }

        case AST_ENUM_DECL: {
            TypeInfo *et = type_named(node->as.enum_decl.name);
            et->base = HTYPE_ENUM;
            scope_define(c, node->as.enum_decl.name, et, false);
            break;
        }

        case AST_IMPL_BLOCK: {
            for (int i = 0; i < node->as.impl_block.method_count; i++)
                analyze_stmt(c, node->as.impl_block.methods[i]);
            break;
        }

        case AST_RETURN: {
            if (node->as.ret.value)
                analyze_expr(c, node->as.ret.value);
            break;
        }

        case AST_IF: {
            analyze_expr(c, node->as.if_stmt.cond);
            if (node->as.if_stmt.then_block)
                analyze_block(c, node->as.if_stmt.then_block);
            if (node->as.if_stmt.else_block)
                analyze_stmt(c, node->as.if_stmt.else_block);
            break;
        }

        case AST_WHILE: {
            analyze_expr(c, node->as.while_stmt.cond);
            if (node->as.while_stmt.body)
                analyze_block(c, node->as.while_stmt.body);
            break;
        }

        case AST_FOR: {
            analyze_expr(c, node->as.for_stmt.iter);
            scope_push(c);
            /* define loop variable - type is element type of iter */
            scope_define(c, node->as.for_stmt.var, type_new(HTYPE_INT), true);
            if (node->as.for_stmt.body) {
                for (int i = 0; i < node->as.for_stmt.body->as.block.count; i++)
                    analyze_stmt(c, node->as.for_stmt.body->as.block.stmts[i]);
            }
            scope_pop(c);
            break;
        }

        case AST_LOOP: {
            if (node->as.loop_stmt.body)
                analyze_block(c, node->as.loop_stmt.body);
            break;
        }

        case AST_BLOCK:
            analyze_block(c, node);
            break;

        case AST_MATCH: {
            analyze_expr(c, node->as.match_stmt.value);
            for (int i = 0; i < node->as.match_stmt.arm_count; i++) {
                ASTNode *arm = node->as.match_stmt.arms[i];
                if (!arm) continue;
                analyze_expr(c, arm->as.match_arm.pattern);
                if (arm->as.match_arm.guard)
                    analyze_expr(c, arm->as.match_arm.guard);
                if (arm->as.match_arm.body) {
                    if (arm->as.match_arm.body->type == AST_BLOCK)
                        analyze_block(c, arm->as.match_arm.body);
                    else
                        analyze_expr(c, arm->as.match_arm.body);
                }
            }
            break;
        }

        case AST_EXPR_STMT:
            if (node->as.block.count > 0)
                analyze_expr(c, node->as.block.stmts[0]);
            break;

        case AST_IMPORT:
        case AST_USE:
        case AST_BREAK:
        case AST_CONTINUE:
            break; /* Nothing to analyze */

        default:
            analyze_expr(c, node);
            break;
    }
}

/* ============================================================
 * ENTRY POINT
 * ============================================================ */
bool semantic_analyze(HolexaCompiler *c, ASTNode *ast) {
    if (!ast) return false;

    /* Initialize global scope */
    c->scope       = scope_new(NULL);
    c->scope_depth = 0;

    /* Pre-define built-in functions */
    scope_define(c, "print",     type_new(HTYPE_VOID), false);
    scope_define(c, "println",   type_new(HTYPE_VOID), false);
    scope_define(c, "input",     type_new(HTYPE_STRING), false);
    scope_define(c, "len",       type_new(HTYPE_INT), false);
    scope_define(c, "exit",      type_new(HTYPE_VOID), false);
    scope_define(c, "assert",    type_new(HTYPE_VOID), false);
    scope_define(c, "typeof",    type_new(HTYPE_STRING), false);
    scope_define(c, "to_string", type_new(HTYPE_STRING), false);
    scope_define(c, "to_int",    type_new(HTYPE_INT), false);
    scope_define(c, "to_float",  type_new(HTYPE_FLOAT), false);
    scope_define(c, "push",      type_new(HTYPE_VOID), false);
    scope_define(c, "pop",       type_new(HTYPE_UNKNOWN), false);
    scope_define(c, "format",    type_new(HTYPE_STRING), false);

    /* Analyze all top-level statements */
    for (int i = 0; i < ast->as.block.count; i++)
        analyze_stmt(c, ast->as.block.stmts[i]);

    return !error_has(c);
}
