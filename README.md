<div align="center">

```
  ██╗  ██╗ ██████╗ ██╗     ███████╗██╗  ██╗ █████╗
  ██║  ██║██╔═══██╗██║     ██╔════╝╚██╗██╔╝██╔══██╗
  ███████║██║   ██║██║     █████╗   ╚███╔╝ ███████║
  ██╔══██║██║   ██║██║     ██╔══╝   ██╔██╗ ██╔══██║
  ██║  ██║╚██████╔╝███████╗███████╗██╔╝ ██╗██║  ██║
  ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝
```

**The HOLEXA Programming Language**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](CHANGELOG.md)
[![GitHub Stars](https://img.shields.io/github/stars/HOLEXA-LANG/holexa-lang?style=social)](https://github.com/HOLEXA-LANG/holexa-lang)

*Readable like Python · Fast like C · Safe like Rust · Simple like Go*

</div>

---

## What is HOLEXA?

HOLEXA (HLEXA) is a modern, statically-typed programming language designed to combine the best qualities of today's most popular languages — without their biggest drawbacks.

| Goal | Status |
|------|--------|
| Python-style readability | ✅ |
| C-level performance | ✅ |
| Rust-inspired safety | ✅ |
| Go-like simplicity | ✅ |
| Cross-platform | ✅ |

---

## Quick Start

### Install (Linux / macOS)

```bash
git clone https://github.com/HOLEXA-LANG/holexa-lang.git
cd holexa-lang/compiler
make
sudo make install
```

### Your First HOLEXA Program

Create `hello.hlx`:

```holexa
fn main() {
    let name = "World"
    println("Hello, " + name + "!")
}
```

Run it:

```bash
hlxc run hello.hlx
```

---

## Language Features

```holexa
// Variables
let name = "HOLEXA"
let mut count = 0

// Functions
fn greet(name: String) -> String {
    return "Hello, " + name + "!"
}

// Control Flow
if count > 10 {
    println("big")
} else {
    println("small")
}

// Loops
for i in 0..10 {
    println(i)
}

while count < 5 {
    count = count + 1
}

// Structs
struct Point {
    x: int,
    y: int,
}

// Pattern Matching
match score {
    100 => println("Perfect!"),
    _   => println("Keep going!"),
}
```

---

## Compiler Commands

| Command | Description |
|---------|-------------|
| `hlxc file.hlx` | Compile to binary |
| `hlxc run file.hlx` | Compile and run |
| `hlxc check file.hlx` | Type-check only |
| `hlxc build` | Build project |
| `hlxc ast file.hlx` | Show AST (debug) |
| `hlxc version` | Show version |
| `hlxc help` | Show help |

---

## iPad / Mobile Development

```bash
# On any Linux shell app (iSH, Termux, etc.)
git clone https://github.com/HOLEXA-LANG/holexa-lang.git
cd holexa-lang/compiler
make
./hlxc run examples/hello.hlx
```

---

## Project Structure

```
holexa-lang/
├── compiler/          # hlxc compiler (C)
│   ├── src/
│   │   ├── holexa.h   # Types, AST, tokens
│   │   ├── lexer.c    # Tokenizer
│   │   ├── parser.c   # Recursive descent parser
│   │   ├── semantic.c # Type checker
│   │   ├── codegen.c  # C code generator
│   │   ├── error.c    # Error system
│   │   └── main.c     # Compiler driver
│   └── Makefile
├── stdlib/            # Standard library
├── examples/          # Example programs
├── docs/              # Documentation
├── tests/             # Test suite
└── editor-support/    # IDE/editor plugins
```

---

## Roadmap

- [x] v1.0.0 — Core language, compiler, basic stdlib
- [ ] v1.1.0 — Generics, traits, async/await
- [ ] v1.2.0 — Package manager (hlxpm)
- [ ] v1.3.0 — LLVM backend
- [ ] v2.0.0 — Self-hosting compiler

---

## Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md).

```bash
git clone https://github.com/HOLEXA-LANG/holexa-lang.git
cd holexa-lang/compiler
make
make test
```

---

## License

MIT — see [LICENSE](LICENSE)

---

<div align="center">
Made with ❤️ by the HOLEXA community<br>
<a href="https://github.com/HOLEXA-LANG/holexa-lang">GitHub</a> · 
<a href="https://github.com/HOLEXA-LANG/holexa-lang/discussions">Discussions</a> · 
<a href="https://github.com/HOLEXA-LANG/holexa-lang/issues">Issues</a>
</div>
