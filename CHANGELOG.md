# CHANGELOG

## v0.1.4 (2020-09-11)

### Fixed bugs

- fixed compile error with a combination of clang & libstdc++.
- fixed crashes on returning if as expression in function.

## Improvement

- introduced address sanitizer and simple fuzzing test in develop environments.
- memory allocations on user-code are freed on a destructor call of runtime.
    - heap allocations by user-code was not freed in previous versions but it's not a serious problem because a program itself are exited when a destructor of runtime is called. This improvement makes sense mostly for a fuzzing test that iterates compilations many times in same process. 

```rust
// mathmatical operators have higher precedence than if or else!
myvar = if(cond) 100+20 else 200*50
```


## v0.1.3 (2020-09-09)

### fixed bugs

- fixed unusual crash when if-statement used on linux 
- fixed operator precedence of if statement when used as expression

```rust
// mathmatical operators have higher precedence than if or else!
myvar = if(cond) 100+20 else 200*50
```

## v0.1.2 (2020-09-07)

### fixed bugs

- Previous release did not contain llvm generation for if statement. Now if statement/expression works correctly.
- fixed a bug that comment-out which ends with a line break could not be parsed correctly.

### Refactoring

MIR(Mid-Level Representation)-related classes' implementation has been made more simplified.
As like ast classes refined in v0.1.0, data does not have member functions and is initiallized with aggregate initialization.

## v0.1.1 (2020-09-05)

### Fixed Bug

This release mainly fixed a parser.

Now, a specification of **block** , statements inside curly-brackets is similar to Rust.

The block can be treated as just an **expression**, like function call or infix operations, when you put a single expression on last line of statements in block, or put `return` statement. That value becomes the block's value itself.

Thus, all examples below is valid.

```rust
fn test(x,y){
    localvar = x*y+100
    localvar //last line is expression.
}
fn test(x,y){
    localvar = x*y+100
    return localvar //last line is return statement.
}

testvar = { velylocal = 100
            verylocal*2004} * 300 // this is also valid...
```

By this change, `if` statement can be both of an expression and a statement. When the contents of then/else statement do not have return value, it is just treated as like function of `void` type.


## v0.1.0 (2020-08-21)

This release is mostly refactoring of internal processings, especialy AST(Abstract Syntax Tree)-related classes which was written in very early stage of development and is completely different style of implementation compared to newly introduced class such as MIR and Type related classes. A dynamic polymorphism in this compiler development is unified to the way using `std::variant`, not an class inheritance.

The release does neither contains new features nor breaking changes but fixed many instabilities, particularly incorrectness of type inference.

Also, build on Ubuntu has prepared. You can check dependencies from [github action workflow page](https://github.com/mimium-org/mimium/actions/runs/217956579/workflow). Seeking people who can test on windows!

## v0.0.0 (2020-02-15)

first release! no arrays, no structs, no include/modules. This is a "proof of concept" version of mimium.

### known bugs

- Calling same functions that uses `self` more than twice, instantiation of self will not be performed properly. (can avoid this behaviour by defining alias of function)
- in `dsp` function, at least 1 global variable should be used.

### limitations

- `if` can have only a expression, not multiple statements(it's like conditional operator(`(cond)? then_expr : else_expr`) in other languages.)
- array access `[]` can be used only for audio files loaded by using `loadwav(path)`. No boundary checks for array access. You can get a size of the array by `loadwavsize(path)`.