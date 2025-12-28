# Sus

Sus (Spaceless Utils Suite) is a **WIP** C++23 utility library for my own projects. No external deps. No historical baggage -- I will make breaking API changes if needed.

## Components

- Debug macros
```cpp
    ASSERT(x != y, "x should not be equal to y", x, y)；
    DASSERT(x > y);
    UNREACHABLE();
    UNREACHABLE("reason", var);
    UNIMPLEMENTED();
    UNIMPLEMENTED("reason", var);
```

- Logging
```cpp
    LOG(INFO, interval(0.5))("{:.2f}", x);
    LOG(ERROR, every(10)) << x;
    LOG_IF(WARNING, x == y, first(3))("{}, {}", SHOW(x), SHOW(y));
    DLOG(DEBUG, interval(2), first(10))("");
    DLOG_IF(FATAL, x + y != 0)("Wrong!") << x + y;
```

- Progress bar
```cpp
    auto p = logging_progress_bar::create("name");
    p->set_range(-10, 10);
    p->add(1);
    auto sub_p = p->sub(6);
    sub_p->set_range(3);
    sub_p->add(2);
    ASSERT(p->value() == (-10 + 1 + 6 * 2 / 3));
```

- Bit flags enum class
```cpp
    template<> struct is_bit_flags<my_enum> : std::true_type {};
    my_enum a = my_enum::essentials, b = my_enum::all;
    ~a, a & b, a | b, a ^ b, a &= b, a |= b, a ^= b;
    contains_all(a, option), contains_all<my_enum::some>(a);
    contains_any(a, option), contains_any<my_enum::some>(a);
    contains_none(a, option), contains_none<my_enum::some>(a);
```

- Other random stuff
    - accumulator: accurate summation algorithm
    - stop_watch/timer
    - macro helpers

## Small talk

This is my personal "missing from the STL" library. I want a single place with the parts I need so I can write toy projects without hunting down a bunch of extra libs. Even with modern import management, pulling in new deps can still be... not fun. This is also a way for me to learn and maybe give something back to open source.

It started in my other projects ([dicer](https://github.com/SpacelessL/dicer) and [big_rat](https://github.com/SpacelessL/big_rat)). Across my stuff I follow a simple rule: aim for the best code-performance / code-joy ratio. I decide what's necessary and what's not. A lot of great open source projects ship 10-50x more code (and sometimes 10x more hacks) to cover edge cases or squeeze a bit more perf that only a small slice of users will ever need. That is awesome and why they are great, but it takes a lot of time. On top of that, big open source libs carry historical baggage and wide compiler support needs, so they rarely get to rewrite or redesign APIs around newer C++ features (like std::format, ranges, string_view, and other C++20/23 stuff). Since this is a personal project, I don't need to cover everything or support old/uncommon compilers. I can lean on modern C++ and break APIs when it helps.

I know myself: I lose interest quickly when things get tedious. If you're someone who can push projects to the limit, I'm jealous. This one is for me, so I do what makes me feel comfortable. Still, I think others might benefit: the code should stay relatively short and (hopefully) clean, so it's easier to read than the big popular ones. New learners might pick up a few ideas.

If I have time, I'll write a few notes on what I learned while building it -- both as a record for me and maybe a small learning resource for someone else.

## TODO

- Crash handler
- Ensure logging correctness if the program crashes
- Inline logging progress bar impl
- Arguments
- Benchmark framework
- Unit test framework
- Fast hash map?
- Parallel computing
- enum, maybe
- maybe a single header version

## License

MIT License - see LICENSE file for details.
