/*
 * HOLEXA Code Generator v1.0.0
 * Emits C99 code from AST  →  GCC compiles to binary
 */

#include "holexa.h"

/* ── helpers ─────────────────────────────────────────────── */
static void emit(FILE *f, const char *fmt, ...) {
    va_list a; va_start(a, fmt); vfprintf(f, fmt, a); va_end(a);
}
static void indent(FILE *f, int d) {
    for (int i = 0; i < d; i++) fputs("    ", f);
}

static int g_tmp   = 0;
static int g_label = 0;
static int new_tmp(void)   { return g_tmp++;   }
static int new_label(void) { return g_label++;  }

static void emit_cstr(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if      (c == '"')  fputs("\\\"", f);
        else if (c == '\\') fputs("\\\\", f);
        else if (c == '\n') fputs("\\n",  f);
        else if (c == '\t') fputs("\\t",  f);
        else if (c == '\r') fputs("\\r",  f);
        else                fputc(c, f);
    }
    fputc('"', f);
}

/* ── operator to C string ────────────────────────────────── */
static const char *op2c(TokenType op) {
    switch (op) {
    case TOK_PLUS:       return "+";
    case TOK_MINUS:      return "-";
    case TOK_STAR:       return "*";
    case TOK_SLASH:      return "/";
    case TOK_PERCENT:    return "%";
    case TOK_EQEQ:       return "==";
    case TOK_NEQ:        return "!=";
    case TOK_LT:         return "<";
    case TOK_GT:         return ">";
    case TOK_LTE:        return "<=";
    case TOK_GTE:        return ">=";
    case TOK_AND:        return "&&";
    case TOK_OR:         return "||";
    case TOK_NOT:        return "!";
    case TOK_BAND:       return "&";
    case TOK_BOR:        return "|";
    case TOK_BXOR:       return "^";
    case TOK_BNOT:       return "~";
    case TOK_LSHIFT:     return "<<";
    case TOK_RSHIFT:     return ">>";
    case TOK_PLUS_EQ:    return "+=";
    case TOK_MINUS_EQ:   return "-=";
    case TOK_STAR_EQ:    return "*=";
    case TOK_SLASH_EQ:   return "/=";
    case TOK_PERCENT_EQ: return "%=";
    case TOK_EQ:         return "=";
    default:             return "?";
    }
}

/* ── C type from TypeInfo ─────────────────────────────────── */
static const char *ctype(TypeInfo *t) {
    if (!t) return "long long";
    switch (t->base) {
    case HTYPE_VOID:   return "void";
    case HTYPE_BOOL:   return "int";
    case HTYPE_INT:
    case HTYPE_I64:    return "long long";
    case HTYPE_I32:    return "int";
    case HTYPE_I16:    return "short";
    case HTYPE_I8:     return "signed char";
    case HTYPE_U64:    return "unsigned long long";
    case HTYPE_U32:    return "unsigned int";
    case HTYPE_U16:    return "unsigned short";
    case HTYPE_U8:     return "unsigned char";
    case HTYPE_FLOAT:
    case HTYPE_F64:    return "double";
    case HTYPE_F32:    return "float";
    case HTYPE_CHAR:   return "char";
    case HTYPE_BYTE:   return "unsigned char";
    case HTYPE_STRING: return "char*";
    case HTYPE_ARRAY:  return "hlx_array*";
    case HTYPE_NONE:   return "void*";
    case HTYPE_STRUCT:
    case HTYPE_UNKNOWN:
    default:           return t->name[0] ? t->name : "long long";
    }
}

/* ── param C type (for function signatures) ──────────────── */
static const char *param_ctype(ASTNode *p) {
    if (!p || !p->as.param.type_node) return "long long";
    const char *n = p->as.param.type_node->as.type_name.name;
    if (!n || n[0] == '\0') return "long long";
    if (strcmp(n,"String")==0 || strcmp(n,"str")==0)   return "char*";
    if (strcmp(n,"int")==0    || strcmp(n,"i64")==0)   return "long long";
    if (strcmp(n,"i32")==0)                             return "int";
    if (strcmp(n,"i16")==0)                             return "short";
    if (strcmp(n,"i8")==0)                              return "signed char";
    if (strcmp(n,"u64")==0)                             return "unsigned long long";
    if (strcmp(n,"u32")==0)                             return "unsigned int";
    if (strcmp(n,"float")==0  || strcmp(n,"f64")==0)   return "double";
    if (strcmp(n,"f32")==0)                             return "float";
    if (strcmp(n,"bool")==0)                            return "int";
    if (strcmp(n,"void")==0)                            return "void";
    if (strcmp(n,"Array")==0)                           return "hlx_array*";
    return "long long";
}

/* ── return C type for fn decl ───────────────────────────── */
static const char *fn_ret_ctype(ASTNode *fn) {
    if (!fn->as.fn_decl.return_type) return "void";
    const char *n = fn->as.fn_decl.return_type->as.type_name.name;
    if (!n || n[0] == '\0') return "void";
    if (strcmp(n,"String")==0 || strcmp(n,"str")==0)  return "char*";
    if (strcmp(n,"int")==0    || strcmp(n,"i64")==0)  return "long long";
    if (strcmp(n,"i32")==0)                            return "int";
    if (strcmp(n,"float")==0  || strcmp(n,"f64")==0)  return "double";
    if (strcmp(n,"bool")==0)                           return "int";
    if (strcmp(n,"void")==0)                           return "void";
    if (strcmp(n,"Array")==0)                          return "hlx_array*";
    return "long long";
}

/* forward decl */
static void emit_expr(FILE *f, ASTNode *n);
static void emit_stmt(FILE *f, ASTNode *n, int d);

/* ── expression emitter ──────────────────────────────────── */
static void emit_expr(FILE *f, ASTNode *n) {
    if (!n) { emit(f, "0"); return; }
    switch (n->type) {

    case AST_INT_LIT:    emit(f, "%lldLL", n->as.int_lit.value); break;
    case AST_FLOAT_LIT:  emit(f, "%g",     n->as.float_lit.value); break;
    case AST_STRING_LIT: emit_cstr(f, n->as.string_lit.value); break;
    case AST_BOOL_LIT:   emit(f, "%d",     n->as.bool_lit.value ? 1 : 0); break;
    case AST_NONE_LIT:   emit(f, "NULL"); break;

    case AST_IDENT:
        emit(f, "hlx_%s", n->as.ident.name);
        break;

    case AST_BINARY: {
        TokenType op = n->as.binary.op;
        /* Power */
        if (op == TOK_POWER) {
            emit(f, "(long long)pow((double)(");
            emit_expr(f, n->as.binary.left);
            emit(f, "),(double)(");
            emit_expr(f, n->as.binary.right);
            emit(f, "))");
            break;
        }
        /* String concat */
        if (op == TOK_PLUS) {
            TypeInfo *lt = n->as.binary.left  ? n->as.binary.left->resolved_type  : NULL;
            TypeInfo *rt = n->as.binary.right ? n->as.binary.right->resolved_type : NULL;
            bool ls = lt && lt->base == HTYPE_STRING;
            bool rs = rt && rt->base == HTYPE_STRING;
            if (ls || rs) {
                emit(f, "hlx_str_concat(");
                if (ls) emit_expr(f, n->as.binary.left);
                else  { emit(f,"hlx_to_str("); emit_expr(f,n->as.binary.left); emit(f,")"); }
                emit(f, ",");
                if (rs) emit_expr(f, n->as.binary.right);
                else  { emit(f,"hlx_to_str("); emit_expr(f,n->as.binary.right); emit(f,")"); }
                emit(f, ")");
                break;
            }
        }
        emit(f, "("); emit_expr(f, n->as.binary.left);
        emit(f, " %s ", op2c(op));
        emit_expr(f, n->as.binary.right); emit(f, ")");
        break;
    }

    case AST_UNARY:
        emit(f, "(%s", op2c(n->as.unary.op));
        emit_expr(f, n->as.unary.operand);
        emit(f, ")");
        break;

    case AST_ASSIGN:
        emit_expr(f, n->as.assign.target);
        emit(f, " %s ", op2c(n->as.assign.op));
        emit_expr(f, n->as.assign.value);
        break;

    case AST_FIELD:
        emit_expr(f, n->as.field.object);
        emit(f, ".%s", n->as.field.field);
        break;

    case AST_INDEX:
        emit(f, "hlx_array_get(");
        emit_expr(f, n->as.index_expr.object);
        emit(f, ","); emit_expr(f, n->as.index_expr.index);
        emit(f, ")");
        break;

    case AST_RANGE: {
        emit(f, "hlx_make_range(");
        emit_expr(f, n->as.range.start);
        emit(f, ",");
        emit_expr(f, n->as.range.end);
        emit(f, ",%d)", n->as.range.inclusive ? 1 : 0);
        break;
    }

    case AST_ARRAY_LIT: {
        emit(f, "hlx_arr_of(%d", n->as.array_lit.count);
        for (int i = 0; i < n->as.array_lit.count; i++) {
            emit(f, ",(long long)("); emit_expr(f, n->as.array_lit.elems[i]); emit(f, ")");
        }
        emit(f, ")");
        break;
    }

    case AST_AWAIT:
        emit_expr(f, n->as.await_expr.expr);
        break;

    case AST_CALL: {
        ASTNode *cal = n->as.call.callee;

        /* ── built-in functions ── */
        if (cal && cal->type == AST_IDENT) {
            const char *nm = cal->as.ident.name;

            /* print / println */
            if (strcmp(nm,"print")==0 || strcmp(nm,"println")==0) {
                int nl = strcmp(nm,"println")==0;
                if (n->as.call.arg_count == 0) {
                    emit(f, nl ? "puts(\"\")" : "fflush(stdout)");
                    break;
                }
                ASTNode *arg = n->as.call.args[0];
                TypeInfo *at = arg ? arg->resolved_type : NULL;
                HolexaType base = at ? at->base : HTYPE_INT;
                if (base == HTYPE_STRING) {
                    emit(f, nl ? "puts(" : "fputs(");
                    emit_expr(f, arg);
                    emit(f, nl ? ")" : ",stdout)");
                } else if (base == HTYPE_FLOAT || base == HTYPE_F64 || base == HTYPE_F32) {
                    emit(f, "printf(\"%s\",", nl ? "%g\\n" : "%g");
                    emit(f, "(double)("); emit_expr(f, arg); emit(f, "))");
                } else if (base == HTYPE_BOOL) {
                    emit(f, "printf(\"%s\",", nl ? "%s\\n" : "%s");
                    emit(f, "("); emit_expr(f, arg); emit(f, ")? \"true\":\"false\")");
                } else {
                    emit(f, "printf(\"%s\",", nl ? "%lld\\n" : "%lld");
                    emit(f, "(long long)("); emit_expr(f, arg); emit(f, "))");
                }
                break;
            }

            if (strcmp(nm,"exit")==0) {
                emit(f, "exit(");
                if (n->as.call.arg_count>0) emit_expr(f, n->as.call.args[0]);
                else emit(f,"0");
                emit(f, ")");
                break;
            }
            if (strcmp(nm,"len")==0 && n->as.call.arg_count>0) {
                ASTNode *a0 = n->as.call.args[0];
                TypeInfo *t0 = a0 ? a0->resolved_type : NULL;
                if (t0 && t0->base==HTYPE_STRING) {
                    emit(f, "(long long)strlen("); emit_expr(f,a0); emit(f,")");
                } else {
                    emit(f, "hlx_array_len("); emit_expr(f,a0); emit(f,")");
                }
                break;
            }
            if (strcmp(nm,"to_string")==0 && n->as.call.arg_count>0) {
                emit(f,"hlx_to_str("); emit_expr(f,n->as.call.args[0]); emit(f,")");
                break;
            }
            if (strcmp(nm,"to_int")==0 && n->as.call.arg_count>0) {
                emit(f,"atoll("); emit_expr(f,n->as.call.args[0]); emit(f,")");
                break;
            }
            if (strcmp(nm,"to_float")==0 && n->as.call.arg_count>0) {
                emit(f,"atof("); emit_expr(f,n->as.call.args[0]); emit(f,")");
                break;
            }
            if (strcmp(nm,"input")==0) {
                if (n->as.call.arg_count>0) {
                    emit(f,"(printf(\"%%s\","); emit_expr(f,n->as.call.args[0]);
                    emit(f,"),hlx_input_line())");
                } else {
                    emit(f,"hlx_input_line()");
                }
                break;
            }
            if (strcmp(nm,"assert")==0 && n->as.call.arg_count>0) {
                emit(f,"do{if(!("); emit_expr(f,n->as.call.args[0]);
                emit(f,")){fprintf(stderr,\"Assertion failed (line %d)\\n\"); exit(1);}}while(0)", n->line);
                break;
            }
        }

        /* ── method calls ── */
        if (cal && cal->type == AST_FIELD) {
            const char *meth = cal->as.field.field;
            ASTNode *obj = cal->as.field.object;
            if (strcmp(meth,"push")==0) {
                emit(f,"hlx_array_push("); emit_expr(f,obj);
                for (int i=0;i<n->as.call.arg_count;i++){ emit(f,","); emit_expr(f,n->as.call.args[i]); }
                emit(f,")"); break;
            }
            if (strcmp(meth,"len")==0) {
                TypeInfo *ot = obj ? obj->resolved_type : NULL;
                if (ot && ot->base==HTYPE_STRING){ emit(f,"(long long)strlen("); emit_expr(f,obj); emit(f,")"); }
                else { emit(f,"hlx_array_len("); emit_expr(f,obj); emit(f,")"); }
                break;
            }
            if (strcmp(meth,"contains")==0 && n->as.call.arg_count>0) {
                emit(f,"hlx_str_contains("); emit_expr(f,obj); emit(f,","); emit_expr(f,n->as.call.args[0]); emit(f,")"); break;
            }
            if (strcmp(meth,"to_upper")==0){ emit(f,"hlx_str_upper("); emit_expr(f,obj); emit(f,")"); break; }
            if (strcmp(meth,"to_lower")==0){ emit(f,"hlx_str_lower("); emit_expr(f,obj); emit(f,")"); break; }
            if (strcmp(meth,"trim")==0)    { emit(f,"hlx_str_trim(");  emit_expr(f,obj); emit(f,")"); break; }
        }

        /* generic call */
        emit_expr(f, cal);
        emit(f,"(");
        for (int i=0;i<n->as.call.arg_count;i++) {
            if (i>0) emit(f,",");
            emit_expr(f,n->as.call.args[i]);
        }
        emit(f,")");
        break;
    }

    default:
        emit(f,"0/*expr%d*/",n->type);
    }
}

/* ── statement emitter ───────────────────────────────────── */
static void emit_stmt(FILE *f, ASTNode *n, int d) {
    if (!n) return;
    switch (n->type) {

    case AST_VAR_DECL:
    case AST_CONST_DECL: {
        indent(f,d);
        TypeInfo *t = n->resolved_type;
        const char *ct = ctype(t);
        emit(f, "%s hlx_%s", ct, n->as.var_decl.name);
        if (n->as.var_decl.init) { emit(f," = "); emit_expr(f, n->as.var_decl.init); }
        else {
            if (t && t->base==HTYPE_STRING)     emit(f," = \"\"");
            else if (t && t->base==HTYPE_FLOAT) emit(f," = 0.0");
            else if (t && t->base==HTYPE_ARRAY) emit(f," = hlx_array_new()");
            else                                emit(f," = 0");
        }
        emit(f,";\n");
        break;
    }

    case AST_FN_DECL: {
        if (!n->as.fn_decl.name[0]) break; /* anonymous */
        const char *ret = fn_ret_ctype(n);
        emit(f,"\n");
        indent(f,d);
        emit(f,"%s hlx_%s(", ret, n->as.fn_decl.name);
        for (int i=0;i<n->as.fn_decl.param_count;i++) {
            ASTNode *p = n->as.fn_decl.params[i];
            if (!p) continue;
            if (i>0) emit(f,",");
            emit(f,"%s hlx_%s", param_ctype(p), p->as.param.name);
        }
        emit(f,") {\n");
        if (n->as.fn_decl.body) {
            for (int i=0;i<n->as.fn_decl.body->as.block.count;i++)
                emit_stmt(f, n->as.fn_decl.body->as.block.stmts[i], d+1);
        }
        indent(f,d); emit(f,"}\n");
        break;
    }

    case AST_STRUCT_DECL: {
        emit(f,"\ntypedef struct {\n");
        for (int i=0;i<n->as.struct_decl.field_count;i++) {
            ASTNode *fld = n->as.struct_decl.fields[i];
            if (!fld) continue;
            indent(f,1);
            const char *fn2 = fld->as.param.type_node ? fld->as.param.type_node->as.type_name.name : "int";
            if      (strcmp(fn2,"String")==0) emit(f,"char* %s;\n", fld->as.param.name);
            else if (strcmp(fn2,"float")==0 || strcmp(fn2,"f64")==0) emit(f,"double %s;\n", fld->as.param.name);
            else if (strcmp(fn2,"bool")==0)   emit(f,"int %s;\n", fld->as.param.name);
            else                              emit(f,"long long %s;\n", fld->as.param.name);
        }
        emit(f,"} %s;\n", n->as.struct_decl.name);
        break;
    }

    case AST_ENUM_DECL: {
        emit(f,"\ntypedef enum {\n");
        for (int i=0;i<n->as.enum_decl.variant_count;i++) {
            ASTNode *v = n->as.enum_decl.variants[i];
            if (!v) continue;
            indent(f,1); emit(f,"%s_%s,\n", n->as.enum_decl.name, v->as.ident.name);
        }
        emit(f,"} %s;\n", n->as.enum_decl.name);
        break;
    }

    case AST_IMPL_BLOCK:
        for (int i=0;i<n->as.impl_block.method_count;i++)
            emit_stmt(f, n->as.impl_block.methods[i], d);
        break;

    case AST_RETURN:
        indent(f,d); emit(f,"return");
        if (n->as.ret.value) { emit(f," "); emit_expr(f, n->as.ret.value); }
        emit(f,";\n");
        break;

    case AST_IF:
        indent(f,d); emit(f,"if ("); emit_expr(f, n->as.if_stmt.cond); emit(f,") {\n");
        if (n->as.if_stmt.then_block)
            for (int i=0;i<n->as.if_stmt.then_block->as.block.count;i++)
                emit_stmt(f, n->as.if_stmt.then_block->as.block.stmts[i], d+1);
        indent(f,d); emit(f,"}");
        if (n->as.if_stmt.else_block) {
            if (n->as.if_stmt.else_block->type == AST_IF) {
                emit(f," else "); emit_stmt(f, n->as.if_stmt.else_block, 0); return;
            }
            emit(f," else {\n");
            for (int i=0;i<n->as.if_stmt.else_block->as.block.count;i++)
                emit_stmt(f, n->as.if_stmt.else_block->as.block.stmts[i], d+1);
            indent(f,d); emit(f,"}");
        }
        emit(f,"\n");
        break;

    case AST_WHILE:
        indent(f,d); emit(f,"while ("); emit_expr(f, n->as.while_stmt.cond); emit(f,") {\n");
        if (n->as.while_stmt.body)
            for (int i=0;i<n->as.while_stmt.body->as.block.count;i++)
                emit_stmt(f, n->as.while_stmt.body->as.block.stmts[i], d+1);
        indent(f,d); emit(f,"}\n");
        break;

    case AST_FOR: {
        ASTNode *iter = n->as.for_stmt.iter;
        indent(f,d);
        if (iter && iter->type == AST_RANGE) {
            int lbl = new_label();
            emit(f,"{ long long _e%d = ", lbl); emit_expr(f, iter->as.range.end);
            emit(f,"; for (long long hlx_%s = ", n->as.for_stmt.var);
            emit_expr(f, iter->as.range.start);
            emit(f,"; hlx_%s %s _e%d; hlx_%s++) {\n",
                 n->as.for_stmt.var,
                 iter->as.range.inclusive ? "<=" : "<",
                 lbl, n->as.for_stmt.var);
        } else {
            int lbl = new_label();
            emit(f,"{ hlx_array* _a%d = ", lbl); emit_expr(f, iter);
            emit(f,"; for (long long _i%d=0; _i%d<hlx_array_len(_a%d); _i%d++) {", lbl,lbl,lbl,lbl);
            emit(f," long long hlx_%s = hlx_array_get(_a%d,_i%d);\n", n->as.for_stmt.var, lbl, lbl);
        }
        if (n->as.for_stmt.body)
            for (int i=0;i<n->as.for_stmt.body->as.block.count;i++)
                emit_stmt(f, n->as.for_stmt.body->as.block.stmts[i], d+1);
        indent(f,d); emit(f,"} }\n");
        break;
    }

    case AST_LOOP:
        indent(f,d); emit(f,"while(1) {\n");
        if (n->as.loop_stmt.body)
            for (int i=0;i<n->as.loop_stmt.body->as.block.count;i++)
                emit_stmt(f, n->as.loop_stmt.body->as.block.stmts[i], d+1);
        indent(f,d); emit(f,"}\n");
        break;

    case AST_BREAK:    indent(f,d); emit(f,"break;\n");    break;
    case AST_CONTINUE: indent(f,d); emit(f,"continue;\n"); break;

    case AST_BLOCK:
        indent(f,d); emit(f,"{\n");
        for (int i=0;i<n->as.block.count;i++) emit_stmt(f, n->as.block.stmts[i], d+1);
        indent(f,d); emit(f,"}\n");
        break;

    case AST_MATCH: {
        int tmp = new_tmp();
        indent(f,d); emit(f,"{ long long _m%d=(long long)(", tmp); emit_expr(f, n->as.match_stmt.value); emit(f,");\n");
        for (int i=0;i<n->as.match_stmt.arm_count;i++) {
            ASTNode *arm = n->as.match_stmt.arms[i];
            if (!arm) continue;
            indent(f,d+1);
            bool wildcard = arm->as.match_arm.pattern &&
                            arm->as.match_arm.pattern->type == AST_IDENT &&
                            strcmp(arm->as.match_arm.pattern->as.ident.name,"_")==0;
            if (!wildcard) { emit(f,"if(_m%d==(long long)(",tmp); emit_expr(f,arm->as.match_arm.pattern); emit(f,"))"); }
            emit(f," {\n");
            if (arm->as.match_arm.body) {
                if (arm->as.match_arm.body->type==AST_BLOCK)
                    for (int j=0;j<arm->as.match_arm.body->as.block.count;j++)
                        emit_stmt(f, arm->as.match_arm.body->as.block.stmts[j], d+2);
                else { indent(f,d+2); emit_expr(f,arm->as.match_arm.body); emit(f,";\n"); }
            }
            indent(f,d+1); emit(f,"}\n");
        }
        indent(f,d); emit(f,"}\n");
        break;
    }

    case AST_IMPORT: case AST_USE: break;

    case AST_EXPR_STMT:
        if (n->as.block.count>0 && n->as.block.stmts[0]) {
            indent(f,d); emit_expr(f, n->as.block.stmts[0]); emit(f,";\n");
        }
        break;

    default:
        indent(f,d); emit(f,"/* stmt %d */\n", n->type);
    }
}

/* ── forward declarations ────────────────────────────────── */
static void emit_fwd_decls(FILE *f, ASTNode *prog) {
    for (int i=0;i<prog->as.block.count;i++) {
        ASTNode *n = prog->as.block.stmts[i];
        if (!n || n->type!=AST_FN_DECL || !n->as.fn_decl.name[0]) continue;
        const char *ret = fn_ret_ctype(n);
        emit(f,"%s hlx_%s(", ret, n->as.fn_decl.name);
        for (int j=0;j<n->as.fn_decl.param_count;j++) {
            ASTNode *p = n->as.fn_decl.params[j];
            if (!p) continue;
            if (j>0) emit(f,",");
            emit(f,"%s hlx_%s", param_ctype(p), p->as.param.name);
        }
        emit(f,");\n");
    }
    emit(f,"\n");
}

/* ── embedded C runtime ──────────────────────────────────── */
static void emit_runtime(FILE *f) {
    fputs(
"/* HOLEXA Runtime v1.0.0 */\n"
"#include <stdio.h>\n"
"#include <stdlib.h>\n"
"#include <string.h>\n"
"#include <stdarg.h>\n"
"#include <stdint.h>\n"
"#include <math.h>\n"
"#include <ctype.h>\n"
"\n"
"typedef struct { long long *data; long long len,cap; } hlx_array;\n"
"\n"
"static hlx_array *hlx_array_new(void){\n"
"    hlx_array *a=malloc(sizeof(hlx_array));\n"
"    a->data=malloc(8*sizeof(long long)); a->len=0; a->cap=8; return a;\n"
"}\n"
"static void hlx_array_push(hlx_array *a,long long v){\n"
"    if(a->len>=a->cap){a->cap*=2;a->data=realloc(a->data,a->cap*sizeof(long long));}\n"
"    a->data[a->len++]=v;\n"
"}\n"
"static long long hlx_array_get(hlx_array *a,long long i){\n"
"    if(!a||i<0||i>=a->len){fprintf(stderr,\"Index out of bounds: %lld\\n\",i);exit(1);}\n"
"    return a->data[i];\n"
"}\n"
"static long long hlx_array_len(hlx_array *a){return a?a->len:0;}\n"
"static hlx_array *hlx_arr_of(int n,...){\n"
"    hlx_array *a=hlx_array_new(); va_list ap; va_start(ap,n);\n"
"    for(int i=0;i<n;i++) hlx_array_push(a,va_arg(ap,long long));\n"
"    va_end(ap); return a;\n"
"}\n"
"static hlx_array *hlx_make_range(long long s,long long e,int inc){\n"
"    hlx_array *a=hlx_array_new();\n"
"    for(long long i=s;inc?i<=e:i<e;i++) hlx_array_push(a,i);\n"
"    return a;\n"
"}\n"
"static char *hlx_str_concat(const char *a,const char *b){\n"
"    if(!a)a=\"\"; if(!b)b=\"\";\n"
"    size_t la=strlen(a),lb=strlen(b);\n"
"    char *r=malloc(la+lb+1); memcpy(r,a,la); memcpy(r+la,b,lb); r[la+lb]=0; return r;\n"
"}\n"
"static char *hlx_to_str(long long v){\n"
"    char *b=malloc(32); snprintf(b,32,\"%lld\",v); return b;\n"
"}\n"
"static int hlx_str_contains(const char *h,const char *n){return strstr(h,n)!=NULL;}\n"
"static char *hlx_str_upper(const char *s){\n"
"    char *r=strdup(s); for(int i=0;r[i];i++) r[i]=toupper((unsigned char)r[i]); return r;\n"
"}\n"
"static char *hlx_str_lower(const char *s){\n"
"    char *r=strdup(s); for(int i=0;r[i];i++) r[i]=tolower((unsigned char)r[i]); return r;\n"
"}\n"
"static char *hlx_str_trim(const char *s){\n"
"    while(isspace((unsigned char)*s))s++;\n"
"    char *r=strdup(s); int l=(int)strlen(r);\n"
"    while(l>0&&isspace((unsigned char)r[l-1]))r[--l]=0; return r;\n"
"}\n"
"static char *hlx_input_line(void){\n"
"    char buf[4096];\n"
"    if(!fgets(buf,sizeof(buf),stdin))return strdup(\"\");\n"
"    size_t l=strlen(buf); if(l>0&&buf[l-1]=='\\n')buf[--l]=0;\n"
"    return strdup(buf);\n"
"}\n\n",
    f);
}

/* ── codegen entry ───────────────────────────────────────── */
bool codegen_emit(HolexaCompiler *c, ASTNode *ast, FILE *out) {
    if (!ast || !out) return false;
    g_tmp=0; g_label=0;

    emit_runtime(out);

    /* structs/enums first */
    for (int i=0;i<ast->as.block.count;i++) {
        ASTNode *n=ast->as.block.stmts[i];
        if (n && (n->type==AST_STRUCT_DECL||n->type==AST_ENUM_DECL))
            emit_stmt(out,n,0);
    }

    /* forward declarations */
    emit_fwd_decls(out, ast);

    /* global vars */
    for (int i=0;i<ast->as.block.count;i++) {
        ASTNode *n=ast->as.block.stmts[i];
        if (n && (n->type==AST_VAR_DECL||n->type==AST_CONST_DECL))
            emit_stmt(out,n,0);
    }

    /* functions */
    for (int i=0;i<ast->as.block.count;i++) {
        ASTNode *n=ast->as.block.stmts[i];
        if (n && n->type==AST_FN_DECL)
            emit_stmt(out,n,0);
    }

    /* main() */
    emit(out,"\nint main(int argc,char **argv){\n    (void)argc;(void)argv;\n");
    for (int i=0;i<ast->as.block.count;i++) {
        ASTNode *n=ast->as.block.stmts[i];
        if (!n) continue;
        switch(n->type) {
        case AST_FN_DECL: case AST_STRUCT_DECL: case AST_ENUM_DECL:
        case AST_VAR_DECL: case AST_CONST_DECL:
        case AST_IMPORT:  case AST_USE:
            break;
        default:
            emit_stmt(out,n,1);
        }
    }
    emit(out,"    return 0;\n}\n");

    return !error_has(c);
}
