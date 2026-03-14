# HOLEXA Compiler Build System
# Usage:
#   make          - Build compiler
#   make install  - Install to /usr/local/bin
#   make test     - Run all tests
#   make clean    - Clean build files

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS = -lm

SRC_DIR = src
BUILD   = build
BIN     = hlxc

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/lexer.c \
       $(SRC_DIR)/parser.c \
       $(SRC_DIR)/semantic.c \
       $(SRC_DIR)/codegen.c \
       $(SRC_DIR)/error.c

OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD)/%.o, $(SRCS))

.PHONY: all clean install uninstall test

all: $(BUILD) $(BIN)
	@echo ""
	@echo "  ✓ HOLEXA compiler built successfully → ./$(BIN)"
	@echo "  Run: ./hlxc help"
	@echo ""

$(BUILD):
	mkdir -p $(BUILD)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/holexa.h
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(BIN)
	@echo "Installing hlxc to /usr/local/bin..."
	cp $(BIN) /usr/local/bin/hlxc
	chmod +x /usr/local/bin/hlxc
	@echo "✓ hlxc installed. Run: hlxc help"

uninstall:
	rm -f /usr/local/bin/hlxc
	@echo "✓ hlxc uninstalled."

test: $(BIN)
	@echo "Running HOLEXA tests..."
	@cd ../tests && bash run_tests.sh

clean:
	rm -rf $(BUILD) $(BIN)
	@echo "✓ Cleaned."

# Quick test targets
test-hello: $(BIN)
	@echo "--- Hello World Test ---"
	@echo 'fn main() { println("Hello, HOLEXA!") }' > /tmp/test_hello.hlx
	./$(BIN) run /tmp/test_hello.hlx
	@rm -f /tmp/test_hello.hlx /tmp/test_hello

test-loop: $(BIN)
	@echo "--- Loop Test ---"
	@printf 'for i in 0..5 {\n    println(i)\n}\n' > /tmp/test_loop.hlx
	./$(BIN) run /tmp/test_loop.hlx
	@rm -f /tmp/test_loop.hlx /tmp/test_loop
