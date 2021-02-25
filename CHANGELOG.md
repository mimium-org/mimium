# CHANGELOG

## v0.3.1(2021-02-25)
### Bugfixes

- fixed a bug in code generator when if statement has void block(#51)
- added missing `==` and `!=` infix operators (#53)

### Refactoring

- `ExecutionEngine` class has been made by splitting from `Runtime` class toward implementing interpreter backend and environment variables feature.

### New Function

Though this release is a patch release, a subtle new feature is added.

- `mem`, a 1-sample delay function has been added. A simple example of biquad filter using this function is added to `mimium-core/filter.mmm`(#44).

### Other updates

A list of all contributers is added to readme by using [all-contributors](https://allcontributors.org/).
## v0.3.0 (2021-02-03)

### New language features

#### Tuple and Array type

New aggregate type, tuple and array have been added.

Tuple type can be constructed with parenthesis and comma.
Currently, the only way to get value of tuple type is like C++'s structural binding.
```rust
# Tuple Construction
triples = (100,200,300)//type signature is (float,float,float)

one,two,three = triples
```
In the future, dot access operator like `triples.0` will be added.

Array type can be constructed with angle brackets and comma.
Currently All the array is mutable , fixed sized and declared as a private global variable in llvm module.
As a unique feature, an interpolation with floating pointer index is supported as same as reading audio files.
```rust
# Array Construction
myarr = [100,200,300]
internalv = myarr[0]//zero-based index
myarr[2] = 400//the array is now [100,200,400]
interp = myarr[0.5]//the result will be 150(linear interpolation)
```

#### Multichannel support for dsp function

Now mimium supports more than mono output, stereo and more many channels.

Numbers of inputs and outputs are determined by a type of `dsp function`.
For example, stereo panning can be written like below.

```rust
fn panner(input:float,pan:float) -> (float,float){
    return (input*pan,input*(1-pan))
}

fn dsp(input:(float,float))->(float,float){
    src = random()*0.2
    res = panner(src,sin(5*now/48000)+1*0.5)
    return res
}
```
This is the breaking change because, 1. input parameter for `dsp` was `time` before v0.2 (this was a temporary solution before `now` was implemented), 2. output type for `dsp` was `float` but now the type for `dsp` should be  `(tuple of float)->(tuple of float)`, a function that takes 1 variable with tuple of floats and returns tuple of floats.
Even if you want to process mono, the output type should be a tuple of 1 float.

Some examples were rewritten to match with this language spec.

### Using mimium as C++ library

Dependencies of library headers and frontend(application instance and CLI) were tidied up. 
We can now use mimium as C++ Library.
You can import mimium easily by using CMake. 
A minimal example is on https://github.com/mimium-org/mimium-libimport-example.

### other changes

- Supported version of LLVM is now 11.
- Code_Of_Conduct.md was added.
## v0.2.0 (2020-12-24)
### Improvements

- Windows build(on MSYS2) is ready. Check GitHub Actions workflow if you want to build.
- `delay(input,delaytime)` primitive function has been added.
  - maximum delay time is currently fixed to 44100 samples.
- (experimental) a simple macro preprocessor has been added.
  - you can now include other source files with `include "otherfile.mmm"` put on a global context.
- Refactored Mid-Level internal representation.

## v0.1.5 (2020-09-28)

### Fixed bugs

- fixed a compile error when if statement contains only a single fcall inside braces(Thanks @t-sin).

### Improvements

- Automatically copies changelog to release body in GitHub actions

### Other

Now new features such as delay primitive function which will be included in v0.2.0 are under development.

## v0.1.4 (2020-09-11)

### Fixed bugs

- fixed compile error with a combination of clang & libstdc++.
- fixed crashes on returning if as expression in function.

## Improvement

- introduced address sanitizer and simple fuzzing test in develop environments.
- memory allocations on user-code are freed on a destructor call of runtime.
    - heap allocations by user-code was not freed in previous versions but it's not a serious problem because a program itself are exited when a destructor of runtime is called. This improvement makes sense mostly for a fuzzing test that iterates compilations many times in same process. 

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