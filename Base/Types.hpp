#pragma once

using i8 = signed char;
using i16 = short;
using i32 = int;
using i64 = long long;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using f32 = float;
using f64 = double;

using usize = decltype(sizeof(0));

template <typename T>
struct Is_Number
{
  static constexpr bool VALUE = false;
};
#define X(T)                            \
  template <>                           \
  struct Is_Number<T>                   \
  {                                     \
    static constexpr bool VALUE = true; \
  };
X(i8)
X(i16)
X(i32)
X(i64)
X(u8)
X(u16)
X(u32)
X(u64)
X(f32)
X(f64)
X(long)
X(unsigned long)
#undef X
// long appers to be a distinct type from int. I dunno why

template <typename T>
struct Type_Identity
{
  using Type = T;
};
template <typename T>
using Non_Deduced = typename Type_Identity<T>::Type;

// Pair<i32, bool> Find(...) { return {index, true}; }
// auto [index, found] = Find(...);
template <typename A, typename B>
struct Pair
{
  A first;
  B second;
};

template <typename A, typename B, typename C>
struct Triple
{
  A first;
  B second;
  C third;
};

template <typename A, typename B>
Pair(A, B) -> Pair<A, B>;

template <typename A, typename B, typename C>
Triple(A, B, C) -> Triple<A, B, C>;