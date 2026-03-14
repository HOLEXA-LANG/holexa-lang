# HOLEXA Language Guide

## Table of Contents
1. [Variables](#variables)
2. [Types](#types)
3. [Functions](#functions)
4. [Control Flow](#control-flow)
5. [Loops](#loops)
6. [Strings](#strings)
7. [Structs](#structs)
8. [Enums](#enums)
9. [Pattern Matching](#pattern-matching)
10. [Error Handling](#error-handling)

---

## Variables

```holexa
let name = "HOLEXA"        // immutable by default
let mut count = 0          // mutable with mut
let pi: float = 3.14159    // explicit type annotation
const MAX: int = 100       // constant
```

## Types

| Type | Description | Example |
|------|-------------|---------|
| `int` | 64-bit integer | `let x = 42` |
| `float` | 64-bit float | `let f = 3.14` |
| `String` | UTF-8 string | `let s = "hi"` |
| `bool` | Boolean | `let b = true` |
| `i8..i64` | Signed integers | `let n: i32 = 100` |
| `u8..u64` | Unsigned integers | `let n: u64 = 100` |
| `f32/f64` | Floating point | `let f: f32 = 1.0` |
| `Array<T>` | Dynamic array | `let a = [1, 2, 3]` |

## Functions

```holexa
// Basic function
fn add(a: int, b: int) -> int {
    return a + b
}

// String function
fn greet(name: String) -> String {
    return "Hello, " + name + "!"
}

// Recursive function
fn factorial(n: int) -> int {
    if n <= 1 { return 1 }
    return n * factorial(n - 1)
}

// Calling functions
let result = add(10, 32)
println(result)                    // 42
println(greet("World"))           // Hello, World!
println(factorial(5))              // 120
```

## Control Flow

```holexa
// if
if age > 18 {
    println("Adult")
}

// if-else
if score > 50 {
    println("Pass")
} else {
    println("Fail")
}

// if-else if-else
if score > 90 {
    println("A")
} else if score > 80 {
    println("B")
} else if score > 70 {
    println("C")
} else {
    println("F")
}
```

## Loops

```holexa
// For range (exclusive end)
for i in 0..10 {
    println(i)        // 0, 1, 2 ... 9
}

// For range (inclusive end)
for i in 1..=10 {
    println(i)        // 1, 2, 3 ... 10
}

// While loop
let mut x = 0
while x < 10 {
    x = x + 1
}

// Infinite loop with break
let mut i = 0
loop {
    i = i + 1
    if i >= 5 { break }
}

// Continue
for i in 0..10 {
    if i % 2 == 0 { continue }
    println(i)        // 1, 3, 5, 7, 9
}
```

## Strings

```holexa
let s = "Hello, HOLEXA!"

// Concatenation
let full = "Hello" + ", " + "World!"

// Length
let length = len(s)         // 15
let length2 = s.len()       // also works

// Case
let upper = s.to_upper()    // HELLO, HOLEXA!
let lower = s.to_lower()    // hello, holexa!

// Trim
let padded = "  hello  "
let clean = padded.trim()   // "hello"

// Contains
if s.contains("HOLEXA") {
    println("Found!")
}

// Convert
let n = 42
let text = to_string(n)     // "42"
let back = to_int("42")     // 42
let f = to_float("3.14")    // 3.14
```

## Structs

```holexa
struct Point {
    x: int,
    y: int,
}

struct Person {
    name: String,
    age: int,
}

// Creating struct instances
let p = Point { x: 10, y: 20 }
println(p.x)    // 10
println(p.y)    // 20

// Passing structs to functions
fn distance(px: int, py: int) -> int {
    return px * px + py * py
}
println(distance(p.x, p.y))
```

## Enums

```holexa
enum Direction {
    North,
    South,
    East,
    West,
}

enum Status {
    Active,
    Inactive,
    Pending,
}
```

## Pattern Matching

```holexa
let score = 85

match score {
    100  => println("Perfect!"),
    90   => println("Excellent!"),
    80   => println("Good!"),
    _    => println("Keep trying!"),
}

// Match with ranges (using if)
match true {
    (score > 90) => println("A"),
    (score > 80) => println("B"),
    _            => println("C"),
}
```

## Built-in Functions

| Function | Description | Example |
|----------|-------------|---------|
| `println(x)` | Print with newline | `println("hi")` |
| `print(x)` | Print without newline | `print("hi")` |
| `input(prompt)` | Read user input | `let s = input("Name: ")` |
| `len(x)` | Length of string/array | `len("hello")` → 5 |
| `to_string(n)` | Convert to string | `to_string(42)` → "42" |
| `to_int(s)` | Convert to int | `to_int("42")` → 42 |
| `to_float(s)` | Convert to float | `to_float("3.14")` → 3.14 |
| `exit(code)` | Exit program | `exit(0)` |
| `assert(cond)` | Assert condition | `assert(x > 0)` |
