[日本語](./design_proposal.ja.md)


# Overall Structure

```
Source Code　⇆ AST ⇆ Graphical Editor(Separated Process)
                    |
                    → Evaluator →Audio/Midi output

```
# Syntax

### Any kinds of variables can hold time positions

```
input::int@ 1@100[ms] //this means interger one, which is placed on absolute time 100ms(Bracket is time unit, you can create your own unit in code)

1@[100,200,300,450][ms] //you can bind multiple time positions(used in function chain, see below), time will be sorted by earlier number when used, because time proceeds linearly

maybe,
1@[some_generator_function()][sample] you can specify time sequence as function, it can be infinite sequence

```

#### Time Shift Operator <| and |>

`hoge::int@ = 1@100[ms] |> 1[sec] //now hoge is 1@1.1[sec]` 

TODO: how can we shift certain number in time sequence?

`hoge::int@ = 1@[100,200,300][ms] |> [0,1,0][sec]`

Chuck 100 +=> now;
Chronic @+ @-

### Functions will be triggered at the time bound to input variable

TODO how can we manage multiple arguments of function?

When we input variables into function, the function will be scheduled to be triggerd at the time bound to the variable(if not bound, will not be triggerd) 

if we bind multiple time positions to variables, the first time will be consumed.
When we call some function inside the function, that will be scheduled at next index.


```
pitch_index::int = produce((x)=>{x+x},5)//[2,4,6,8,10]
 time_index::int@ = produce((x)=>{x*x},5) // [@1,@4,@9,@16,@25]
 end_index = time_index.map((t)=>{ t |> 2[s]}) //[@3,@6,@11,@18,@27]
 
 
 pitchevents = pitch_index.tbind((x,i)=>{x@time_index[i]@end_index[i]}) //[2@[1,3],4@[4,6],6@[9,11],8@[16,18],10@[25,27]]
 
 function noteOn(input_pitch::int,instrument){//input_pitch is like 2@[1,3] and trigger at time 1
 	instrument.pitch = input_pitch
 	instrument.gain = 1.0
 	bind(noteOff(input_pitch,instrument)) // now input_pitch is 2@3,will triggerd at time 3
 }
  function noteOff(input_pitch::int,instrument){
   	instrument.gain = 0.0
  }
 
 pitchevents.map(pitch=>noteOn(pitch)) // bind noteon to all pitchevents elements
```

### You can refer past/future value of variable in function

```
 
delay(input::int@) = input <| 100[ms] //this output the past input of 100ms:delay of 100ms

future(input::int@) = input |> 100::ms //this output the future input of 100ms(this makes global offset delay like vst plugin system, useful when we build filter)

```

### Function can refer a past of its own output by calling "self"

```
combfilter(input) = input + 0.999*( self<|1 ) // feeding back its output by one index (self cannot refer its future, it will return error)

combfilter = input+0.999*(self <| 1[sec] ) // maybe can we refer past by relative time? if we have built-in interpolation
 
```

The functions in this language can be used like pipeline, however, it internally holds a value like a class for future/past reference.
If compiler can't detect maximum size of past reference, it means we need infinite memory. it will returns compile error(as same as faust's delay operator)


## Additional features

### Built-in Interpolation & boundary treatment

when we have an array like

```
hoge = [20,30,0.12]
```

we can call like `hoge[0.3]`and it outputs between 20 and 30

We can define interpolation function

### Dialect feature

it's like a alias for type/function name but it can be applied once( you cannnot make an alias of alias)

assuming mostly for shorthand used in performance

it will be implemented like a macro so does not affect to run-time performance

But macro in everywhere should not be allowed, it will produce many dirty codes.

sometimes we writes dirty and evil code, but it should be allowed only sometime.




