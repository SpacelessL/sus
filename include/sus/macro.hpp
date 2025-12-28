#pragma once

#define PARENS ()
#define IDENTITY(...) __VA_ARGS__

#define EXPAND(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) __VA_ARGS__

#define FOR_EACH(macro, ...) __VA_OPT__(EXPAND(FOR_EACH_HELPER(macro, __VA_ARGS__)))
#define FOR_EACH_HELPER(macro, a1, ...) macro(a1) __VA_OPT__(FOR_EACH_AGAIN PARENS (macro, __VA_ARGS__))
#define FOR_EACH_AGAIN() FOR_EACH_HELPER

#define OVERLOADED(...) [&](auto &&...args) -> decltype(auto) { return (__VA_ARGS__)(std::forward<decltype(args)>(args)...); }

#define FIRST_ONLY(x, ...) x
#define FIRST_IGNORED(x, ...) __VA_ARGS__

#define SUS_DETAIL_CONCAT2_IMPL(x, y) x##y

#define CONCAT2(x, y) SUS_DETAIL_CONCAT2_IMPL(x, y)

#define ANON(prefix) CONCAT2(_anon_##prefix##_, __COUNTER__)
