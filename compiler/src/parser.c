/*
 * HOLEXA Parser
 * Recursive descent parser → produces AST
 */

#include "holexa.h"

/* ============================================================
 * PARSER HELPERS
 * ============================================================ */

/* Advance past newlines */
static void skip_newlines(HolexaCompiler *c) {
    while (c->current.type == TOK_NEWLINE) {
        c->current = c->peek;
        c->peek    = lexer_next(c);
    }
}

static Token cur(HolexaCompiler *c)  { return c->current; }
static Token pek(HolexaCompiler *c)  { return c->peek; }

static Token advance_tok(HolexaCompiler *c) {
    Token t    = c->current;
    c->current = c->peek;
    c->peek    = lexer_next(c);
    /* skip newlines transparently in most contexts */
    while (c->current.type == TOK_NEWLINE) {
        c->current = c->peek;
        c->peek    = lexer_next(c);
    }
    return t;
}

static bool check(HolexaCompiler *c, TokenType t) {
    return c->current.type == t;
}

static bool match_tok(HolexaCompiler *c, TokenType t) {
    if (check(c, t)) { advance_tok(c); return true; }
    return false;
}

static Token expect(HolexaCompiler *c, TokenType t) {
    if (c->current.type != t) {
        error_add(c, ERR_PARSER, 200, c->current.line, c->current.col,
                  "Expected %s but found %s '%s'",
                  token_type_name(t),
                  token_type_name(c->current.type),
                  c->current.value);
        /* Return a fake token to keep parsing */
        return c->current;
    }
    return advance_tok(c);
}

ASTNode *ast_node_new(HolexaCompiler *c, ASTNodeType type, int line, int col) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    if (!n) {
        fprintf(stderr, "Fatal: out of memory\n");
        exit(1);
    }
    n->type = type;
    n->line = line;
    n->col  = col;
    return n;
}

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */
static ASTNode *parse_stmt(HolexaCompiler *c);
static ASTNode *parse_expr(HolexaCompiler *c);
static ASTNode *parse_type(HolexaCompiler *c);
static ASTNode *parse_block(HolexaCompiler *c);
static ASTNode *parse_fn_decl(HolexaCompiler *c, bool is_pub, bool is_async);
static ASTNode *parse_struct_decl(HolexaCompiler *c, bool is_pub);
static ASTNode *parse_enum_decl(HolexaCompiler *c, bool is_pub);

/* ============================================================
 * TYPE PARSER
 * ============================================================ */
static ASTNode *parse_type(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    ASTNode *n = ast_node_new(c, AST_TYPE_NAME, line, col);

    /* Optional / nullable: ?Type */
    if (match_tok(c, TOK_QUESTION)) {
        ASTNode *inner = parse_type(c);
        n->type = AST_TYPE_OPT;
        n->as.type_name.params[0] = inner;
        n->as.type_name.param_count = 1;
        strncpy(n->as.type_name.name, "Optional", MAX_IDENTIFIER-1);
        return n;
    }

    /* Reference: &Type */
    if (match_tok(c, TOK_BAND)) {
        ASTNode *inner = parse_type(c);
        n->type = AST_TYPE_REF;
        n->as.type_name.params[0] = inner;
        n->as.type_name.param_count = 1;
        strncpy(n->as.type_name.name, "Ref", MAX_IDENTIFIER-1);
        return n;
    }

    /* Array: [Type] or [Type; N] */
    if (match_tok(c, TOK_LBRACKET)) {
        ASTNode *inner = parse_type(c);
        expect(c, TOK_RBRACKET);
        n->type = AST_TYPE_ARRAY;
        n->as.type_name.params[0] = inner;
        n->as.type_name.param_count = 1;
        strncpy(n->as.type_name.name, "Array", MAX_IDENTIFIER-1);
        return n;
    }

    /* Function type: fn(T, T) -> T */
    if (check(c, TOK_FN)) {
        advance_tok(c);
        n->type = AST_TYPE_FN;
        strncpy(n->as.type_name.name, "fn", MAX_IDENTIFIER-1);
        expect(c, TOK_LPAREN);
        while (!check(c, TOK_RPAREN) && !check(c, TOK_EOF)) {
            if (n->as.type_name.param_count > 0) expect(c, TOK_COMMA);
            n->as.type_name.params[n->as.type_name.param_count++] = parse_type(c);
        }
        expect(c, TOK_RPAREN);
        if (match_tok(c, TOK_ARROW)) {
            n->as.type_name.params[n->as.type_name.param_count++] = parse_type(c);
        }
        return n;
    }

    /* Named type with optional generics: Ident or Ident<T, U> */
    Token name_tok = cur(c);
    /* Accept any identifier or built-in type keyword as type name */
    strncpy(n->as.type_name.name, name_tok.value, MAX_IDENTIFIER-1);
    advance_tok(c);

    /* Generics: Name<T, U> */
    if (check(c, TOK_LT)) {
        advance_tok(c);
        while (!check(c, TOK_GT) && !check(c, TOK_EOF)) {
            if (n->as.type_name.param_count > 0) expect(c, TOK_COMMA);
            n->as.type_name.params[n->as.type_name.param_count++] = parse_type(c);
        }
        expect(c, TOK_GT);
    }

    return n;
}

/* ============================================================
 * EXPRESSION PARSER (Pratt / precedence climbing)
 * ============================================================ */

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,   /* = += -= etc */
    PREC_OR,       /* || */
    PREC_AND,      /* && */
    PREC_BITOR,    /* | */
    PREC_BITXOR,   /* ^ */
    PREC_BITAND,   /* & */
    PREC_EQ,       /* == != */
    PREC_CMP,      /* < > <= >= */
    PREC_SHIFT,    /* << >> */
    PREC_ADD,      /* + - */
    PREC_MUL,      /* * / % */
    PREC_UNARY,    /* ! - ~ */
    PREC_POWER,    /* ** */
    PREC_CALL,     /* () [] . */
    PREC_PRIMARY,
} Precedence;

static int get_precedence(TokenType t) {
    switch (t) {
        case TOK_EQ: case TOK_PLUS_EQ: case TOK_MINUS_EQ:
        case TOK_STAR_EQ: case TOK_SLASH_EQ: case TOK_PERCENT_EQ:
            return PREC_ASSIGN;
        case TOK_OR:      return PREC_OR;
        case TOK_AND:     return PREC_AND;
        case TOK_BOR:     return PREC_BITOR;
        case TOK_BXOR:    return PREC_BITXOR;
        case TOK_BAND:    return PREC_BITAND;
        case TOK_EQEQ: case TOK_NEQ:
            return PREC_EQ;
        case TOK_LT: case TOK_GT: case TOK_LTE: case TOK_GTE:
            return PREC_CMP;
        case TOK_LSHIFT: case TOK_RSHIFT:
            return PREC_SHIFT;
        case TOK_DOTDOT: case TOK_DOTDOTEQ:
            return PREC_ADD;
        case TOK_PLUS: case TOK_MINUS:
            return PREC_ADD;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT:
            return PREC_MUL;
        case TOK_POWER:
            return PREC_POWER;
        case TOK_LPAREN: case TOK_LBRACKET: case TOK_DOT: case TOK_DOUBLECOLON:
            return PREC_CALL;
        default: return PREC_NONE;
    }
}

static bool is_assign_op(TokenType t) {
    return t == TOK_EQ || t == TOK_PLUS_EQ || t == TOK_MINUS_EQ ||
           t == TOK_STAR_EQ || t == TOK_SLASH_EQ || t == TOK_PERCENT_EQ;
}

static ASTNode *parse_primary(HolexaCompiler *c);
static ASTNode *parse_expr_prec(HolexaCompiler *c, int min_prec);

static ASTNode *parse_primary(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    Token t = cur(c);

    /* Integer literal */
    if (t.type == TOK_INT_LIT) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_INT_LIT, line, col);
        n->as.int_lit.value = atoll(t.value);
        return n;
    }

    /* Float literal */
    if (t.type == TOK_FLOAT_LIT) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_FLOAT_LIT, line, col);
        n->as.float_lit.value = atof(t.value);
        return n;
    }

    /* String literal */
    if (t.type == TOK_STRING_LIT) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_STRING_LIT, line, col);
        strncpy(n->as.string_lit.value, t.value, MAX_STRING_LEN - 1);
        return n;
    }

    /* Bool literals */
    if (t.type == TOK_TRUE) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_BOOL_LIT, line, col);
        n->as.bool_lit.value = true;
        return n;
    }
    if (t.type == TOK_FALSE) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_BOOL_LIT, line, col);
        n->as.bool_lit.value = false;
        return n;
    }

    /* None literal */
    if (t.type == TOK_NONE) {
        advance_tok(c);
        return ast_node_new(c, AST_NONE_LIT, line, col);
    }

    /* Identifier */
    if (t.type == TOK_IDENT || t.type == TOK_SELF) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_IDENT, line, col);
        strncpy(n->as.ident.name, t.value, MAX_IDENTIFIER - 1);
        return n;
    }

    /* Grouped expression: ( expr ) */
    if (t.type == TOK_LPAREN) {
        advance_tok(c);
        ASTNode *inner = parse_expr(c);
        expect(c, TOK_RPAREN);
        return inner;
    }

    /* Array literal: [a, b, c] */
    if (t.type == TOK_LBRACKET) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_ARRAY_LIT, line, col);
        while (!check(c, TOK_RBRACKET) && !check(c, TOK_EOF)) {
            if (n->as.array_lit.count > 0) expect(c, TOK_COMMA);
            n->as.array_lit.elems[n->as.array_lit.count++] = parse_expr(c);
        }
        expect(c, TOK_RBRACKET);
        return n;
    }

    /* Unary operators: - ! ~ */
    if (t.type == TOK_MINUS || t.type == TOK_NOT || t.type == TOK_BNOT) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_UNARY, line, col);
        n->as.unary.op      = t.type;
        n->as.unary.operand = parse_expr_prec(c, PREC_UNARY);
        return n;
    }

    /* Await expression */
    if (t.type == TOK_AWAIT) {
        advance_tok(c);
        ASTNode *n = ast_node_new(c, AST_AWAIT, line, col);
        n->as.await_expr.expr = parse_expr(c);
        return n;
    }

    /* Anonymous function / closure */
    if (t.type == TOK_FN) {
        return parse_fn_decl(c, false, false);
    }

    /* Error recovery */
    error_add(c, ERR_PARSER, 201, line, col,
              "Unexpected token %s '%s' in expression",
              token_type_name(t.type), t.value);
    advance_tok(c);
    return ast_node_new(c, AST_INT_LIT, line, col); /* dummy */
}

static ASTNode *parse_expr_prec(HolexaCompiler *c, int min_prec) {
    ASTNode *left = parse_primary(c);

    while (true) {
        int prec = get_precedence(cur(c).type);
        if (prec <= min_prec) break;

        int line = cur(c).line, col = cur(c).col;
        TokenType op = cur(c).type;

        /* Function call */
        if (op == TOK_LPAREN) {
            advance_tok(c);
            ASTNode *call = ast_node_new(c, AST_CALL, line, col);
            call->as.call.callee = left;
            while (!check(c, TOK_RPAREN) && !check(c, TOK_EOF)) {
                if (call->as.call.arg_count > 0) expect(c, TOK_COMMA);
                call->as.call.args[call->as.call.arg_count++] = parse_expr(c);
            }
            expect(c, TOK_RPAREN);
            left = call;
            continue;
        }

        /* Index access */
        if (op == TOK_LBRACKET) {
            advance_tok(c);
            ASTNode *idx = ast_node_new(c, AST_INDEX, line, col);
            idx->as.index_expr.object = left;
            idx->as.index_expr.index  = parse_expr(c);
            expect(c, TOK_RBRACKET);
            left = idx;
            continue;
        }

        /* Field / method access */
        if (op == TOK_DOT) {
            advance_tok(c);
            ASTNode *field = ast_node_new(c, AST_FIELD, line, col);
            field->as.field.object = left;
            Token name = expect(c, TOK_IDENT);
            strncpy(field->as.field.field, name.value, MAX_IDENTIFIER - 1);
            left = field;
            continue;
        }

        /* Assignment operators */
        if (is_assign_op(op)) {
            advance_tok(c);
            ASTNode *assign = ast_node_new(c, AST_ASSIGN, line, col);
            assign->as.assign.target = left;
            assign->as.assign.op     = op;
            assign->as.assign.value  = parse_expr_prec(c, PREC_ASSIGN - 1);
            left = assign;
            continue;
        }

        /* Range: start..end or start..=end */
        if (op == TOK_DOTDOT || op == TOK_DOTDOTEQ) {
            advance_tok(c);
            ASTNode *range = ast_node_new(c, AST_RANGE, line, col);
            range->as.range.start     = left;
            range->as.range.inclusive = (op == TOK_DOTDOTEQ);
            range->as.range.end       = parse_expr_prec(c, PREC_ADD);
            left = range;
            continue;
        }

        /* Binary operators */
        advance_tok(c);
        ASTNode *right = parse_expr_prec(c, prec);
        ASTNode *bin   = ast_node_new(c, AST_BINARY, line, col);
        bin->as.binary.op    = op;
        bin->as.binary.left  = left;
        bin->as.binary.right = right;
        left = bin;
    }

    return left;
}

static ASTNode *parse_expr(HolexaCompiler *c) {
    return parse_expr_prec(c, PREC_NONE);
}

/* ============================================================
 * STATEMENT PARSER
 * ============================================================ */

static ASTNode *parse_block(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_LBRACE);
    ASTNode *block = ast_node_new(c, AST_BLOCK, line, col);

    while (!check(c, TOK_RBRACE) && !check(c, TOK_EOF)) {
        ASTNode *s = parse_stmt(c);
        if (s && block->as.block.count < MAX_STMTS)
            block->as.block.stmts[block->as.block.count++] = s;
    }
    expect(c, TOK_RBRACE);
    return block;
}

static ASTNode *parse_if(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_IF);
    ASTNode *n = ast_node_new(c, AST_IF, line, col);
    n->as.if_stmt.cond       = parse_expr(c);
    n->as.if_stmt.then_block = parse_block(c);
    n->as.if_stmt.else_block = NULL;

    if (match_tok(c, TOK_ELSE)) {
        if (check(c, TOK_IF))
            n->as.if_stmt.else_block = parse_if(c);
        else
            n->as.if_stmt.else_block = parse_block(c);
    }
    return n;
}

static ASTNode *parse_while(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_WHILE);
    ASTNode *n = ast_node_new(c, AST_WHILE, line, col);
    n->as.while_stmt.cond = parse_expr(c);
    n->as.while_stmt.body = parse_block(c);
    return n;
}

static ASTNode *parse_for(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_FOR);
    ASTNode *n = ast_node_new(c, AST_FOR, line, col);
    Token var = expect(c, TOK_IDENT);
    strncpy(n->as.for_stmt.var, var.value, MAX_IDENTIFIER - 1);
    expect(c, TOK_IN);
    n->as.for_stmt.iter = parse_expr(c);
    n->as.for_stmt.body = parse_block(c);
    return n;
}

static ASTNode *parse_loop(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_LOOP);
    ASTNode *n = ast_node_new(c, AST_LOOP, line, col);
    n->as.loop_stmt.body = parse_block(c);
    return n;
}

static ASTNode *parse_return(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_RETURN);
    ASTNode *n = ast_node_new(c, AST_RETURN, line, col);
    if (!check(c, TOK_RBRACE) && !check(c, TOK_SEMICOLON) &&
        !check(c, TOK_NEWLINE) && !check(c, TOK_EOF))
        n->as.ret.value = parse_expr(c);
    match_tok(c, TOK_SEMICOLON);
    return n;
}

static ASTNode *parse_let(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_LET);
    ASTNode *n = ast_node_new(c, AST_VAR_DECL, line, col);
    n->as.var_decl.is_mut = match_tok(c, TOK_MUT);
    Token name = expect(c, TOK_IDENT);
    strncpy(n->as.var_decl.name, name.value, MAX_IDENTIFIER - 1);

    if (match_tok(c, TOK_COLON))
        n->as.var_decl.type_node = parse_type(c);

    if (match_tok(c, TOK_EQ))
        n->as.var_decl.init = parse_expr(c);

    match_tok(c, TOK_SEMICOLON);
    return n;
}

static ASTNode *parse_const_decl(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    advance_tok(c); /* consume 'const' */
    ASTNode *n = ast_node_new(c, AST_CONST_DECL, line, col);
    Token name = expect(c, TOK_IDENT);
    strncpy(n->as.var_decl.name, name.value, MAX_IDENTIFIER - 1);
    if (match_tok(c, TOK_COLON))
        n->as.var_decl.type_node = parse_type(c);
    expect(c, TOK_EQ);
    n->as.var_decl.init = parse_expr(c);
    match_tok(c, TOK_SEMICOLON);
    return n;
}

static ASTNode *parse_fn_decl(HolexaCompiler *c, bool is_pub, bool is_async) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_FN);
    ASTNode *n = ast_node_new(c, AST_FN_DECL, line, col);
    n->as.fn_decl.is_pub   = is_pub;
    n->as.fn_decl.is_async = is_async;

    /* Anonymous functions may have no name */
    if (check(c, TOK_IDENT)) {
        Token name = advance_tok(c);
        strncpy(n->as.fn_decl.name, name.value, MAX_IDENTIFIER - 1);
    }

    expect(c, TOK_LPAREN);
    while (!check(c, TOK_RPAREN) && !check(c, TOK_EOF)) {
        if (n->as.fn_decl.param_count > 0) expect(c, TOK_COMMA);

        int pl = cur(c).line, pc = cur(c).col;
        ASTNode *param = ast_node_new(c, AST_PARAM, pl, pc);
        param->as.param.is_mut = match_tok(c, TOK_MUT);

        /* self parameter */
        if (check(c, TOK_SELF)) {
            Token st = advance_tok(c);
            strncpy(param->as.param.name, st.value, MAX_IDENTIFIER-1);
        } else {
            Token pname = expect(c, TOK_IDENT);
            strncpy(param->as.param.name, pname.value, MAX_IDENTIFIER-1);
            if (match_tok(c, TOK_COLON))
                param->as.param.type_node = parse_type(c);
            if (match_tok(c, TOK_EQ))
                param->as.param.default_val = parse_expr(c);
        }
        n->as.fn_decl.params[n->as.fn_decl.param_count++] = param;
    }
    expect(c, TOK_RPAREN);

    if (match_tok(c, TOK_ARROW))
        n->as.fn_decl.return_type = parse_type(c);

    if (check(c, TOK_LBRACE))
        n->as.fn_decl.body = parse_block(c);
    else
        match_tok(c, TOK_SEMICOLON);

    return n;
}

static ASTNode *parse_struct_decl(HolexaCompiler *c, bool is_pub) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_STRUCT);
    ASTNode *n = ast_node_new(c, AST_STRUCT_DECL, line, col);
    n->as.struct_decl.is_pub = is_pub;
    Token name = expect(c, TOK_IDENT);
    strncpy(n->as.struct_decl.name, name.value, MAX_IDENTIFIER-1);

    expect(c, TOK_LBRACE);
    while (!check(c, TOK_RBRACE) && !check(c, TOK_EOF)) {
        int fl = cur(c).line, fc = cur(c).col;
        ASTNode *field = ast_node_new(c, AST_PARAM, fl, fc);
        bool fpub = match_tok(c, TOK_PUB);
        (void)fpub;
        Token fname = expect(c, TOK_IDENT);
        strncpy(field->as.param.name, fname.value, MAX_IDENTIFIER-1);
        expect(c, TOK_COLON);
        field->as.param.type_node = parse_type(c);
        match_tok(c, TOK_COMMA);
        if (n->as.struct_decl.field_count < MAX_MEMBERS)
            n->as.struct_decl.fields[n->as.struct_decl.field_count++] = field;
    }
    expect(c, TOK_RBRACE);
    return n;
}

static ASTNode *parse_enum_decl(HolexaCompiler *c, bool is_pub) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_ENUM);
    ASTNode *n = ast_node_new(c, AST_ENUM_DECL, line, col);
    n->as.enum_decl.is_pub = is_pub;
    Token name = expect(c, TOK_IDENT);
    strncpy(n->as.enum_decl.name, name.value, MAX_IDENTIFIER-1);

    expect(c, TOK_LBRACE);
    while (!check(c, TOK_RBRACE) && !check(c, TOK_EOF)) {
        int vl = cur(c).line, vc = cur(c).col;
        ASTNode *variant = ast_node_new(c, AST_IDENT, vl, vc);
        Token vname = expect(c, TOK_IDENT);
        strncpy(variant->as.ident.name, vname.value, MAX_IDENTIFIER-1);
        match_tok(c, TOK_COMMA);
        if (n->as.enum_decl.variant_count < MAX_MEMBERS)
            n->as.enum_decl.variants[n->as.enum_decl.variant_count++] = variant;
    }
    expect(c, TOK_RBRACE);
    return n;
}

static ASTNode *parse_impl(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_IMPL);
    ASTNode *n = ast_node_new(c, AST_IMPL_BLOCK, line, col);
    Token target = expect(c, TOK_IDENT);
    strncpy(n->as.impl_block.target, target.value, MAX_IDENTIFIER-1);

    /* impl Type for Trait */
    if (check(c, TOK_IDENT) && strcmp(cur(c).value, "for") == 0) {
        advance_tok(c);
        Token trait = expect(c, TOK_IDENT);
        strncpy(n->as.impl_block.trait_name, trait.value, MAX_IDENTIFIER-1);
    }

    expect(c, TOK_LBRACE);
    while (!check(c, TOK_RBRACE) && !check(c, TOK_EOF)) {
        bool mpub   = match_tok(c, TOK_PUB);
        bool masync = match_tok(c, TOK_ASYNC);
        if (check(c, TOK_FN)) {
            ASTNode *method = parse_fn_decl(c, mpub, masync);
            if (n->as.impl_block.method_count < MAX_MEMBERS)
                n->as.impl_block.methods[n->as.impl_block.method_count++] = method;
        } else {
            advance_tok(c); /* skip unknown token in impl */
        }
    }
    expect(c, TOK_RBRACE);
    return n;
}

static ASTNode *parse_import(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    advance_tok(c); /* consume 'import' or 'use' */
    ASTNode *n = ast_node_new(c, AST_IMPORT, line, col);

    /* path can be a::b::c or "path/string" */
    if (check(c, TOK_STRING_LIT)) {
        Token path = advance_tok(c);
        strncpy(n->as.import.path, path.value, MAX_STRING_LEN-1);
    } else {
        /* Collect identifier path */
        char path[MAX_STRING_LEN] = "";
        while (check(c, TOK_IDENT)) {
            Token seg = advance_tok(c);
            strncat(path, seg.value, MAX_STRING_LEN - strlen(path) - 1);
            if (check(c, TOK_DOUBLECOLON)) {
                advance_tok(c);
                strncat(path, "::", MAX_STRING_LEN - strlen(path) - 1);
            } else break;
        }
        strncpy(n->as.import.path, path, MAX_STRING_LEN-1);
    }

    /* as alias */
    if (check(c, TOK_IDENT) && strcmp(cur(c).value, "as") == 0) {
        advance_tok(c);
        Token alias = expect(c, TOK_IDENT);
        strncpy(n->as.import.alias, alias.value, MAX_IDENTIFIER-1);
    }
    match_tok(c, TOK_SEMICOLON);
    return n;
}

static ASTNode *parse_match(HolexaCompiler *c) {
    int line = cur(c).line, col = cur(c).col;
    expect(c, TOK_MATCH);
    ASTNode *n = ast_node_new(c, AST_MATCH, line, col);
    n->as.match_stmt.value = parse_expr(c);
    expect(c, TOK_LBRACE);

    while (!check(c, TOK_RBRACE) && !check(c, TOK_EOF)) {
        int al = cur(c).line, ac = cur(c).col;
        ASTNode *arm = ast_node_new(c, AST_MATCH_ARM, al, ac);
        arm->as.match_arm.pattern = parse_expr(c);

        /* Guard: if condition */
        if (check(c, TOK_IF)) {
            advance_tok(c);
            arm->as.match_arm.guard = parse_expr(c);
        }

        expect(c, TOK_FAT_ARROW);

        if (check(c, TOK_LBRACE))
            arm->as.match_arm.body = parse_block(c);
        else {
            arm->as.match_arm.body = parse_expr(c);
            match_tok(c, TOK_COMMA);
        }

        if (n->as.match_stmt.arm_count < MAX_STMTS)
            n->as.match_stmt.arms[n->as.match_stmt.arm_count++] = arm;
    }
    expect(c, TOK_RBRACE);
    return n;
}

/* ============================================================
 * PARSE STATEMENT
 * ============================================================ */
static ASTNode *parse_stmt(HolexaCompiler *c) {
    bool is_pub   = false;
    bool is_async = false;
    bool is_extern = false;

    /* Handle pub, async, extern modifiers */
    if (check(c, TOK_PUB))    { is_pub   = true; advance_tok(c); }
    if (check(c, TOK_ASYNC))  { is_async = true; advance_tok(c); }
    if (check(c, TOK_EXTERN)) { is_extern = true; advance_tok(c); (void)is_extern; }

    switch (cur(c).type) {
        case TOK_LET:       return parse_let(c);
        case TOK_CONST:     return parse_const_decl(c);
        case TOK_FN:        return parse_fn_decl(c, is_pub, is_async);
        case TOK_STRUCT:    return parse_struct_decl(c, is_pub);
        case TOK_ENUM:      return parse_enum_decl(c, is_pub);
        case TOK_IMPL:      return parse_impl(c);
        case TOK_IMPORT:    return parse_import(c);
        case TOK_USE:       return parse_import(c);
        case TOK_RETURN:    return parse_return(c);
        case TOK_IF:        return parse_if(c);
        case TOK_WHILE:     return parse_while(c);
        case TOK_FOR:       return parse_for(c);
        case TOK_LOOP:      return parse_loop(c);
        case TOK_MATCH:     return parse_match(c);
        case TOK_BREAK: {
            int l = cur(c).line, co = cur(c).col;
            advance_tok(c);
            match_tok(c, TOK_SEMICOLON);
            return ast_node_new(c, AST_BREAK, l, co);
        }
        case TOK_CONTINUE: {
            int l = cur(c).line, co = cur(c).col;
            advance_tok(c);
            match_tok(c, TOK_SEMICOLON);
            return ast_node_new(c, AST_CONTINUE, l, co);
        }
        case TOK_LBRACE:    return parse_block(c);
        case TOK_SEMICOLON: { advance_tok(c); return NULL; }
        case TOK_EOF:       return NULL;
        default: {
            /* Expression statement */
            ASTNode *expr = parse_expr(c);
            match_tok(c, TOK_SEMICOLON);
            int line = expr ? expr->line : 0;
            int col  = expr ? expr->col  : 0;
            ASTNode *s = ast_node_new(c, AST_EXPR_STMT, line, col);
            s->as.block.stmts[0] = expr;
            s->as.block.count    = 1;
            return s;
        }
    }
}

/* ============================================================
 * PROGRAM PARSER ENTRY
 * ============================================================ */
ASTNode *parser_parse(HolexaCompiler *c) {
    /* Initialize lexer lookahead */
    c->current = lexer_next(c);
    c->peek    = lexer_next(c);
    /* skip leading newlines */
    while (c->current.type == TOK_NEWLINE) {
        c->current = c->peek;
        c->peek    = lexer_next(c);
    }

    ASTNode *program = ast_node_new(c, AST_PROGRAM, 1, 1);

    while (!check(c, TOK_EOF)) {
        ASTNode *s = parse_stmt(c);
        if (s && program->as.block.count < MAX_STMTS)
            program->as.block.stmts[program->as.block.count++] = s;
        if (error_has(c) && c->errors.count >= 10) break; /* stop after too many errors */
    }

    return program;
}
