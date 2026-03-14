# HOLEXA Compiler Guide

## Overview

The HOLEXA compiler (hlxc) converts `.hlx` source files to native binaries.

**Pipeline:**
```
Source (.hlx) → Lexer → Parser → AST → Semantic Analysis → C Code → GCC → Binary
```

## Installation

### From Source

```bash
git clone https://github.com/HOLEXA-LANG/holexa-lang.git
cd holexa-lang/compiler
make
sudo make install      # installs to /usr/local/bin/hlxc
```

### Verify Installation

```bash
hlxc version
hlxc help
```

## Commands

```bash
hlxc file.hlx          # Compile → binary
hlxc run file.hlx      # Compile + run
hlxc check file.hlx    # Type-check only (no binary)
hlxc build             # Build project (reads holexa.toml)
hlxc ast file.hlx      # Show AST (debug)
hlxc tokens file.hlx   # Show tokens (debug)
hlxc version           # Show version
hlxc help              # Show help
```

## Error Codes

| Code | Meaning | Example |
|------|---------|---------|
| HLX100 | Unexpected character | Invalid symbol in source |
| HLX200 | Parse error | Missing `{` or `)` |
| HLX201 | Unexpected token | Wrong syntax |
| HLX203 | Type mismatch | `let x: int = "hello"` |
| HLX302 | Undefined symbol | Using variable before declaring |
| HLX500 | I/O error | Cannot read/write file |

## iPad / Termux Workflow

```bash
# Install Termux (Android) or iSH (iOS)
# Then:
pkg install git gcc make     # Termux
apk add git gcc make         # iSH

git clone https://github.com/HOLEXA-LANG/holexa-lang.git
cd holexa-lang/compiler
make
./hlxc run ../examples/hello.hlx
```
