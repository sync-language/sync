#pragma once
#ifndef SY_UTIL_UNREACHABLE_HPP_
#define SY_UTIL_UNREACHABLE_HPP_

// https://en.cppreference.com/w/c/program/unreachable

// Uses compiler specific extensions if possible.
#ifdef __GNUC__ // GCC, Clang, ICC

#define unreachable() (__builtin_unreachable())

#elif _MSC_VER // MSVC

#define unreachable() (__assume(false))

#else
// Even if no extension is used, undefined behavior is still raised by
// the empty function body and the noreturn attribute.

// The external definition of unreachable_impl must be emitted in a separated TU
// due to the rule for inline functions in C.

[[noreturn]] inline void unreachable_impl() {}
#define unreachable() (unreachable_impl())

#endif // Compiler specific

#endif // SY_UTIL_UNREACHABLE_HPP_