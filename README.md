# Sus

Sus (Spaceless Utils Suite) is a **WIP** C++23 utility library for my own projects. No external deps. No historical baggage -- I will make breaking API changes if needed.

## Components

- Logging
```cpp
while (true) LOG(INFO, interval(0.5))("{:.2f}", x);  // at most once per 0.5 seconds
```
```
[`void namespace::function_name()`
I[thread_id] :2026-01-01 01:01:01.501 +0000 file.cpp(11:3)]: 3.14
[`void namespace::function_name()`
I[thread_id] :2026-01-01 01:01:02.001 +0000 file.cpp(11:3)]: 3.14
[`void namespace::function_name()`
I[thread_id] :2026-01-01 01:01:02.501 +0000 file.cpp(11:3)]: 3.14
```
```cpp
LOG(ERROR, every(10)) << x;  // every 10th call
LOG_IF(WARNING, x == y, first(3))("{}, {}", SHOW(x), SHOW(y));  // only if x == y, first 3 times
DLOG(DEBUG, interval(2), first(10))("");  // debug-only, at most 10 logs, at least 2s apart
DLOG_IF(FATAL, x + y != 0)("Wrong!") << x + y;  // debug-only, conditional
```
```cpp
// expensive_function() is NOT evaluated when:
// - condition is false
// - log level is below threshold
// - DLOG is disabled at compile time
// - rate limiter (interval/every/first) blocks the log
LOG_IF(INFO, false)("{}", expensive_function());
LOG_IF(INFO, false) << expensive_function();
LOG(DEBUG)("{}", expensive_function());       // skipped if level > DEBUG
DLOG(INFO)("{}", expensive_function());       // skipped in release build
LOG(INFO, first(1))("{}", expensive_function());  // skipped after first call
```

- Debug macros
```cpp
ASSERT(x > y, "x should be bigger than y! What are they?", x, y, x > y);
```
```
[`void namespace::function_name()`
C[thread_id] :2026-01-01 01:01:01.001 +0000 file.cpp(11:3)]:
ASSERT(x > y) failed: x should be bigger than y! What are they?
    wherein:
        x:      -1
        y:      2
        x > y:  false
Stacktrace:
       0# namespace::function_name() at :0
       1# main at :0
       2# __libc_start_call_main at ../sysdeps/nptl/libc_start_call_main.h:58
       3# __libc_start_main_impl at ../csu/libc-start.c:360
       4# _start at :0
```
```cpp
DASSERT(x > y);                   // debug-only assert
UNREACHABLE();                    // marks unreachable code path
UNREACHABLE("reason", var);       // with message and debug info
UNIMPLEMENTED();                  // marks unimplemented code path
UNIMPLEMENTED("reason", var);     // with message and debug info
```
```
[`void namespace::function_name()`
C[thread_id] :2026-01-01 01:01:01.001 +0000 file.cpp(11:3)]:
UNIMPLEMENTED: reason
    wherein:
        var:    [var_value]
Stacktrace:
    [Stacktrace info]
```

- Progress bar
```cpp
auto p = logging_progress_bar::create("test", interval(3));
p->set_range(1000000);
for (int i = 0; i < 1000000; ++i) {
    std::this_thread::sleep_for(1ms);
    p->add(1);
}
```
```
[`void namespace::function_name()`
I[thread_id] :2026-01-01 01:01:03.001 +0000 file.cpp(11:3)]: test :
        2808 / 1e+06 (0.28%), 9.36e+02/s
        Elapsed:   3.001 seconds
        Remaining: 17 minutes 45 seconds
[`void namespace::function_name()`
I[thread_id] :2026-01-01 01:01:06.001 +0000 file.cpp(11:3)]: test :
        5619 / 1e+06 (0.56%), 9.37e+02/s
        Elapsed:   6.001 seconds
        Remaining: 17 minutes 41 seconds
```
```cpp
auto p = logging_progress_bar::create("name");
p->set_range(-10, 10);
p->add(1);
// p->value() ≈ -9
auto sub_p = p->sub(6);  // sub-progress that contributes 6 units to parent
sub_p->set_range(3);
sub_p->add(2);
// p->value() ≈ -5 now, which is -10 + 1 + 6 * 2 / 3
```

- Bit flags enum class
```cpp
template<> struct is_bit_flags<my_enum> : std::true_type {};  // opt-in to enable operators
my_enum a = my_enum::essentials, b = my_enum::all;
~a, a & b, a | b, a ^ b, a &= b, a |= b, a ^= b;
contains_all(a, option), contains_all<my_enum::some>(a);
contains_any(a, option), contains_any<my_enum::some>(a);
contains_none(a, option), contains_none<my_enum::some>(a);
```

- Scope guards
```cpp
SCOPE_EXIT([]{ cleanup(); });      // always runs on scope exit
SCOPE_SUCC([&]{ on_success(); });  // runs only if scope exits normally
SCOPE_FAIL([&]{ on_failure(); });  // runs only if scope exits via exception
SCOPE_EXIT(func, arg1, arg2);      // alternative syntax with function and args
```

- Other random stuff
    - accumulator: accurate summation algorithm
    - rate_limit
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
