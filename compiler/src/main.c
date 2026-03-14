/*
 * HOLEXA Compiler - Main Entry Point
 * hlxc - The HOLEXA Compiler
 *
 * Usage:
 *   hlxc <file.hlx>           Compile and produce binary
 *   hlxc run <file.hlx>       Compile and run immediately
 *   hlxc build                Build project from holexa.toml
 *   hlxc check <file.hlx>     Type-check only, no output
 *   hlxc ast <file.hlx>       Print AST (debug)
 *   hlxc tokens <file.hlx>    Print tokens (debug)
 *   hlxc version              Show version
 *   hlxc help                 Show help
 */

#include "holexa.h"
#include <sys/stat.h>
#include <unistd.h>

/* ============================================================
 * COMPILER LIFECYCLE
 * ============================================================ */
HolexaCompiler *compiler_new(const char *filename, const char *source, bool verbose) {
    HolexaCompiler *c = calloc(1, sizeof(HolexaCompiler));
    c->source      = source;
    c->source_len  = strlen(source);
    c->filename    = filename;
    c->pos         = 0;
    c->line        = 1;
    c->col         = 1;
    c->scope       = NULL;
    c->scope_depth = 0;
    c->tmp_var_count = 0;
    c->label_count   = 0;
    c->verbose       = verbose;
    c->debug_ast     = false;
    c->debug_tokens  = false;
    c->optimize      = false;
    c->errors.count  = 0;
    c->errors.has_error = false;
    return c;
}

void compiler_free(HolexaCompiler *c) {
    if (!c) return;
    /* Free all scopes */
    while (c->scope) scope_pop(c);
    free(c);
}

bool compiler_compile(HolexaCompiler *c, const char *out_c_file) {
    if (c->verbose) fprintf(stderr, "  [hlxc] Lexing %s...\n", c->filename);

    /* --- Debug: print tokens --- */
    if (c->debug_tokens) {
        HolexaCompiler *tc = compiler_new(c->filename, c->source, false);
        tc->pos = 0; tc->line = 1; tc->col = 1;
        printf("=== TOKENS ===\n");
        Token t;
        do {
            t = lexer_next(tc);
            printf("  [%s] '%s' (line %d, col %d)\n",
                   token_type_name(t.type), t.value, t.line, t.col);
        } while (t.type != TOK_EOF && t.type != TOK_ERROR);
        compiler_free(tc);
        printf("==============\n\n");
    }

    /* --- Parse --- */
    if (c->verbose) fprintf(stderr, "  [hlxc] Parsing...\n");
    c->ast = parser_parse(c);

    if (error_has(c)) {
        error_print_all(c);
        return false;
    }

    /* --- Debug: print AST --- */
    if (c->debug_ast && c->ast) {
        printf("=== AST ===\n");
        ast_print(c->ast, 0);
        printf("===========\n\n");
    }

    /* --- Semantic analysis --- */
    if (c->verbose) fprintf(stderr, "  [hlxc] Analyzing types...\n");
    semantic_analyze(c, c->ast);

    if (error_has(c)) {
        error_print_all(c);
        return false;
    }

    /* --- Code generation --- */
    if (c->verbose) fprintf(stderr, "  [hlxc] Generating C code...\n");
    FILE *out = fopen(out_c_file, "w");
    if (!out) {
        error_add(c, ERR_IO, 500, 0, 0, "Cannot open output file: %s", out_c_file);
        error_print_all(c);
        return false;
    }

    bool ok = codegen_emit(c, c->ast, out);
    fclose(out);

    if (!ok || error_has(c)) {
        error_print_all(c);
        return false;
    }

    return true;
}

/* ============================================================
 * FILE EXTENSION CHECK
 * ============================================================ */
static bool has_hlx_extension(const char *path) {
    size_t len = strlen(path);
    return len > 4 && strcmp(path + len - 4, ".hlx") == 0;
}

static void strip_extension(const char *path, char *out, size_t out_size) {
    strncpy(out, path, out_size - 1);
    out[out_size - 1] = '\0';
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';
}

/* ============================================================
 * COMPILE & LINK
 * ============================================================ */
static int compile_file(const char *hlx_file, bool run_after,
                        bool check_only, bool verbose,
                        bool debug_ast, bool debug_tokens) {
    /* Read source */
    char *source = read_file(hlx_file);
    if (!source) {
        fprintf(stderr, "\033[1;31m[hlxc] Error:\033[0m Cannot read file: %s\n", hlx_file);
        return 1;
    }

    /* Create compiler */
    HolexaCompiler *c = compiler_new(hlx_file, source, verbose);
    c->debug_ast    = debug_ast;
    c->debug_tokens = debug_tokens;

    /* Generate output paths */
    char base[256];
    strip_extension(hlx_file, base, sizeof(base));

    char c_file[300], bin_file[300];
    snprintf(c_file,   sizeof(c_file),   "%s.hlx.c",  base);
    snprintf(bin_file, sizeof(bin_file), "%s", base);

    printf("\033[1;34m  HOLEXA\033[0m \033[90mv%s\033[0m — Compiling \033[1m%s\033[0m\n",
           HOLEXA_VERSION, hlx_file);

    if (check_only) {
        /* Parse + semantic only */
        c->ast = parser_parse(c);
        if (!error_has(c)) semantic_analyze(c, c->ast);
        if (error_has(c)) {
            error_print_all(c);
            compiler_free(c); free(source);
            return 1;
        }
        printf("\033[1;32m  ✓\033[0m Type check passed.\n");
        compiler_free(c); free(source);
        return 0;
    }

    /* Full compile */
    if (!compiler_compile(c, c_file)) {
        compiler_free(c); free(source);
        return 1;
    }

    /* Invoke GCC */
    char gcc_cmd[1024];
    snprintf(gcc_cmd, sizeof(gcc_cmd),
             "gcc -O2 -o \"%s\" \"%s\" -lm 2>&1",
             bin_file, c_file);

    if (verbose) fprintf(stderr, "  [hlxc] Running: %s\n", gcc_cmd);

    int gcc_ret = system(gcc_cmd);
    remove(c_file); /* clean up C file */

    if (gcc_ret != 0) {
        fprintf(stderr, "\033[1;31m[hlxc] Error:\033[0m GCC compilation failed.\n");
        compiler_free(c); free(source);
        return 1;
    }

    printf("\033[1;32m  ✓\033[0m Compiled → \033[1m%s\033[0m\n", bin_file);

    compiler_free(c);
    free(source);

    if (run_after) {
        printf("\033[1;33m  Running...\033[0m\n\n");
        char run_cmd[400];
        snprintf(run_cmd, sizeof(run_cmd), "\"%s\"", bin_file);
        return system(run_cmd);
    }

    return 0;
}

/* ============================================================
 * HELP & VERSION
 * ============================================================ */
static void print_version(void) {
    printf(
        "\033[1;34m"
        "  ██╗  ██╗ ██████╗ ██╗     ███████╗██╗  ██╗ █████╗ \n"
        "  ██║  ██║██╔═══██╗██║     ██╔════╝╚██╗██╔╝██╔══██╗\n"
        "  ███████║██║   ██║██║     █████╗   ╚███╔╝ ███████║\n"
        "  ██╔══██║██║   ██║██║     ██╔══╝   ██╔██╗ ██╔══██║\n"
        "  ██║  ██║╚██████╔╝███████╗███████╗██╔╝ ██╗██║  ██║\n"
        "  ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝\n"
        "\033[0m"
        "\n"
        "  HOLEXA Programming Language v%s\n"
        "  Compiler: hlxc (C transpiler → GCC)\n"
        "  License: MIT\n"
        "  GitHub:  https://github.com/HOLEXA-LANG/holexa-lang\n"
        "\n"
        "  Philosophy:\n"
        "    Readable like Python · Fast like C\n"
        "    Safe like Rust · Simple like Go\n"
        "\n",
        HOLEXA_VERSION
    );
}

static void print_help(void) {
    print_version();
    printf(
        "  Usage:\n"
        "    hlxc <file.hlx>              Compile to binary\n"
        "    hlxc run <file.hlx>          Compile and run\n"
        "    hlxc check <file.hlx>        Type-check only\n"
        "    hlxc build                   Build from holexa.toml\n"
        "    hlxc ast <file.hlx>          Show AST (debug)\n"
        "    hlxc tokens <file.hlx>       Show tokens (debug)\n"
        "    hlxc version                 Show version\n"
        "    hlxc help                    Show this help\n"
        "\n"
        "  Flags:\n"
        "    --verbose, -v                Verbose output\n"
        "    --optimize, -O              Enable optimizations\n"
        "\n"
        "  Examples:\n"
        "    hlxc hello.hlx\n"
        "    hlxc run hello.hlx\n"
        "    hlxc check myproject.hlx\n"
        "\n"
        "  Error codes:\n"
        "    HLX100  Unexpected character\n"
        "    HLX200  Parse error\n"
        "    HLX201  Unexpected token\n"
        "    HLX203  Type mismatch\n"
        "    HLX302  Undefined symbol\n"
        "    HLX500  I/O error\n"
        "\n"
    );
}

/* ============================================================
 * BUILD COMMAND (simple project build)
 * ============================================================ */
static int cmd_build(bool verbose) {
    /* Look for holexa.toml */
    FILE *f = fopen("holexa.toml", "r");
    if (!f) {
        fprintf(stderr, "\033[1;31m[hlxc]\033[0m No holexa.toml found in current directory.\n");
        fprintf(stderr, "  Create one with: hlxc init\n");
        return 1;
    }
    fclose(f);

    /* Find main.hlx */
    char *source = read_file("src/main.hlx");
    if (!source) source = read_file("main.hlx");
    if (!source) {
        fprintf(stderr, "\033[1;31m[hlxc]\033[0m Cannot find src/main.hlx or main.hlx\n");
        return 1;
    }
    free(source);

    /* Compile */
    char *main_file = "src/main.hlx";
    if (access(main_file, F_OK) != 0) main_file = "main.hlx";
    return compile_file(main_file, false, false, verbose, false, false);
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(int argc, char **argv) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    bool verbose      = false;
    bool debug_ast    = false;
    bool debug_tokens = false;
    bool optimize     = false;

    /* Parse flags */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0)
            verbose = true;
        else if (strcmp(argv[i], "--optimize") == 0 || strcmp(argv[i], "-O") == 0)
            optimize = true;
    }
    (void)optimize;

    const char *cmd = argv[1];

    /* version */
    if (strcmp(cmd, "version") == 0 || strcmp(cmd, "--version") == 0 || strcmp(cmd, "-V") == 0) {
        print_version();
        return 0;
    }

    /* help */
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_help();
        return 0;
    }

    /* build */
    if (strcmp(cmd, "build") == 0) {
        return cmd_build(verbose);
    }

    /* run */
    if (strcmp(cmd, "run") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: hlxc run <file.hlx>\n");
            return 1;
        }
        const char *file = argv[2];
        if (!has_hlx_extension(file)) {
            fprintf(stderr, "\033[1;31m[hlxc]\033[0m File must have .hlx extension: %s\n", file);
            return 1;
        }
        return compile_file(file, true, false, verbose, debug_ast, debug_tokens);
    }

    /* check */
    if (strcmp(cmd, "check") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: hlxc check <file.hlx>\n"); return 1; }
        return compile_file(argv[2], false, true, verbose, debug_ast, debug_tokens);
    }

    /* ast (debug) */
    if (strcmp(cmd, "ast") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: hlxc ast <file.hlx>\n"); return 1; }
        return compile_file(argv[2], false, false, verbose, true, false);
    }

    /* tokens (debug) */
    if (strcmp(cmd, "tokens") == 0) {
        if (argc < 3) { fprintf(stderr, "Usage: hlxc tokens <file.hlx>\n"); return 1; }
        return compile_file(argv[2], false, false, verbose, false, true);
    }

    /* Direct file compile: hlxc file.hlx */
    if (has_hlx_extension(cmd)) {
        return compile_file(cmd, false, false, verbose, debug_ast, debug_tokens);
    }

    fprintf(stderr, "\033[1;31m[hlxc]\033[0m Unknown command: %s\n", cmd);
    fprintf(stderr, "  Run 'hlxc help' for usage.\n");
    return 1;
}
