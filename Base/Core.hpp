#pragma once

#include "Pre.hpp"
#include "Types.hpp"
#include <assert.h>

#define DoNotUseThisGlue(A, B) A##B
#define Glue(A, B) DoNotUseThisGlue(A, B)

#if IS_COMPILER_CLANG
#define PrintfLike(fmtIndex, argsIndex) __attribute__((format(printf, fmtIndex, argsIndex)))
#else
#define PrintfLike(fmtIndex, argsIndex)
#endif

template <typename F>
struct DoNotUseThis_Defer_Scope
{
  F function;

  DoNotUseThis_Defer_Scope(F deferred) : function(deferred) {}
  ~DoNotUseThis_Defer_Scope() { function(); }

  DoNotUseThis_Defer_Scope(const DoNotUseThis_Defer_Scope &) = delete;
  DoNotUseThis_Defer_Scope &operator=(const DoNotUseThis_Defer_Scope &) = delete;
};

#define Defer(...)                                                     \
  auto Glue(doNotUseThisDefer, __COUNTER__) = DoNotUseThis_Defer_Scope \
  {                                                                    \
    [&]() { __VA_ARGS__ }                                              \
  }

template <typename T>
[[nodiscard]] constexpr T Minimum(T a, T b) { return a < b ? a : b; }
template <typename T>
[[nodiscard]] constexpr T Maximum(T a, T b) { return a > b ? a : b; }

[[nodiscard]] constexpr bool IsPow2(usize x) { return (x != 0) && !(x & (x - 1)); }
[[nodiscard]] constexpr bool IsPow2OrZero(usize x) { return !(x & (x - 1)); }

[[nodiscard]]
constexpr usize AlignForwardPow2(usize val, usize alignment)
{
  assert(IsPow2(alignment) && "alignment must be power of two");
  return (val + (alignment - 1)) & ~(alignment - 1);
}

[[nodiscard]]
constexpr usize AlignForward(usize val, usize alignment)
{
  auto result = val + alignment - 1;
  return result - result % alignment;
}

[[nodiscard]]
constexpr u64 MulDivU64(u64 value, u64 numerator, u64 denominator)
{
  u64 quotient = value / denominator;
  u64 remainder = value % denominator;
  return quotient * numerator + remainder * numerator / denominator;
}

[[nodiscard]] constexpr usize Kilobytes(usize n) { return n * 1024; }
[[nodiscard]] constexpr usize Megabytes(usize n) { return Kilobytes(n) * 1024; }
[[nodiscard]] constexpr usize Gigabytes(usize n) { return Megabytes(n) * 1024; }
[[nodiscard]] constexpr usize Terabytes(usize n) { return Gigabytes(n) * 1024; }

[[nodiscard]] constexpr usize Thousand(usize n) { return n * 1000; }
[[nodiscard]] constexpr usize Million(usize n) { return Thousand(n) * 1000; }
[[nodiscard]] constexpr usize Billion(usize n) { return Million(n) * 1000; }
