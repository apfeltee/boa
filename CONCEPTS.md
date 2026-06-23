
# separation of floating point / fixed point numbers

 - currently uses `double` for all numbers, regardless if they're float or not
 - might increase performance
 - *will* increase complexity

# switch / case

same style as in javascript / c, et al.

# dynamic parser / extend and/or modify parser at runtime

 - requires the parser to be present even during execution, incurs overhead
 - a very primitive variation already possible with `eval()`
 - adding new keywords, symbols, et cetera, means dynamic table, rather than static
 - probably not very realistic

# inline assembler / runtime registry execution

 - requires a parserfor a kind of assembly language corresponding to the register opcode shortcodes
 - could be used to alter code interactively, perhaps even self-modifying code
 - syntax maybe: `ASM{ ... }`
 - bit of a stretch, really


## decorators / attributes:

```
[threads(4)]
function iterate(stuff)
{
    ....
}

class Blargh
{
...
}

[]
```
