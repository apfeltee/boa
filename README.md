
Boa is general-purpose scripting language, with all the bells and whistles of a modern language, with special focus on support for one-liners, batch scripting, et cetera.
Boa uses a register-based VM, with a memory pool allocator (based on LuaJIT's `ljalloc.h`), and

  - is easy to extend
  - native-function callbacks are non-nonsensical. you don't need to worry about any stacks, the VM handles it for you
  - full support for `try` `catch` `finally` with builtin Fiber
  - written in C, *but* with  C++ compatibility in mind. that is to say, the sources compile cleanly as C++
  - memory pool can be disabled through command line flags
  - very fast, even with optimizations disabled

TODO:

    - module support. currently, `require` is extremely ad-hoc. but, let's not overdo it either.
    - event processing. could be done with coro, possibly.
    - FFI support. sadly, libffi is not easily integrated in an existing build structure that tries to remain simple. maybe dyncall, or something else.
    - less reasonable: compile to VM-specific assembly; conceptually, it would be as if compiling to bytecode, just, y'know textual.. with an additional parser.