# CHANGELOG

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