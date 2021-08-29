# Roc Lang compiler

This is a just a research compiler project to learn LLVM infrastructure. 
Syntax is similar to `Go` programming language.

Project is divided into 5 modules:

- `parser` -> Lexer & Parser
- `compiler` -> LLVM IR generator
- `linking` -> linker to provide builtin runtime types and functions
- `mir` -> Middle Intermediate Representation, similar to `Rust` language, provides another layer of abstraction to the compiler which can help to add some type checking, memory management or code optimizations
- `passes` -> passes to resolve jump labels, reference increment/decrement etc.

If you want to play around check unit tests in `tests` module.

# Requirements

- LLVM compiler infrastructure
- Clang to link compiled output with code from `linking` module

