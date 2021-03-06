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

# Syntax

You can check the syntax in `sample/source` directory or check the `tests` modules.

Simple `Hello world` example:

```
package main;

fun printHello() {
    println("Hello world");
}

printHello()
```

# Requirements

- LLVM compiler infrastructure
- Clang to link compiled output with code from `linking` module

# Building

In the root directory create a build directory:

```
mkdir mybuilddir
```

Then:

```
cd mybuilddir
```

From the `mybuilddir` execute following command:

```
cmake ..
```

After that execute build command:

```
cmake --build . --target roc-lang
```

This will produce a ready to use compiler executable (depending on build type: in `mybuildir/Debug` or `mybuilddir/Release`).

# Compiling

Given compiler produces an `output.s` file. 
To build a final executable you must link it with runtime API (see `linking/API.cpp` and `linking/API.h`).

Command (`output.s` and `API.cpp`, `API.h` must be in the same directory):

```
clang API.cpp output.s -o myprogram.exe
```

This will produce an `myprogram.exe` Windows executable.

You can run it:

```
I:\>myprogram2
Hello world
```


