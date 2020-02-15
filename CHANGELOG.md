# CHANGELOG

## v0.0.0 (2020-02-15)

first release! no arrays, no structs, no include/modules. This is a "proof of concept" version of mimium.

### known bugs

- Calling same functions that uses `self` more than twice, instantiation of self will not be performed properly. (can avoid this behaviour by defining alias of function)
- in `dsp` function, at least 1 global variable should be used.

### limitations

- `if` can have only a expression, not multiple statements(it's like conditional operator(`(cond)? then_expr : else_expr`) in other languages.)
- array access `[]` can be used only for audio files loaded by using `loadwav(path)`. No boundary checks for array access. You can get a size of the array by `loadwavsize(path)`.