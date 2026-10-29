#pragma once

#include "Core.hpp"
#include <math.h>

[[nodiscard]] inline f32 FloorF32(f32 x) { return floorf(x); }
[[nodiscard]] inline f32 CeilF32(f32 x) { return ceilf(x); }
[[nodiscard]] inline f32 RoundHalfDownF32(f32 x) { return ceilf(x - 0.5f); }
[[nodiscard]] inline f32 RoundHalfUpF32(f32 x) { return floorf(x + 0.5f); }
[[nodiscard]] inline f32 RoundF32(f32 x) { return roundf(x); }
[[nodiscard]] constexpr f32 LerpF32(f32 start, f32 end, f32 t) { return (1 - t) * start + t * end; }

template <typename T>
union Vector2
{
  static_assert(Is_Number<T>::VALUE, "Vector2 needs a numeric type");

  T data[2];

  struct
  {
    T x, y;
  };

  struct
  {
    T width, height;
  };

  // only exist for -Wmissing-braces
  Vector2() = default;
  constexpr Vector2(T left, T right) : data{left, right} {}
};

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator+(Vector2<T> left, Vector2<T> right) { return {left.data[0] + right.data[0], left.data[1] + right.data[1]}; }
template <typename T>
[[nodiscard]] constexpr Vector2<T> operator-(Vector2<T> left, Vector2<T> right) { return {left.data[0] - right.data[0], left.data[1] - right.data[1]}; }
template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(Vector2<T> left, Vector2<T> right) { return {left.data[0] * right.data[0], left.data[1] * right.data[1]}; }
template <typename T>
[[nodiscard]] constexpr Vector2<T> operator/(Vector2<T> left, Vector2<T> right) { return {left.data[0] / right.data[0], left.data[1] / right.data[1]}; }

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator+(Vector2<T> left, Non_Deduced<T> right) { return {left.data[0] + right, left.data[1] + right}; }
template <typename T>
[[nodiscard]] constexpr Vector2<T> operator-(Vector2<T> left, Non_Deduced<T> right) { return {left.data[0] - right, left.data[1] - right}; }
template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(Vector2<T> left, Non_Deduced<T> right) { return {left.data[0] * right, left.data[1] * right}; }
template <typename T>
[[nodiscard]] constexpr Vector2<T> operator/(Vector2<T> left, Non_Deduced<T> right) { return {left.data[0] / right, left.data[1] / right}; }

template <typename T>
constexpr Vector2<T> &operator+=(Vector2<T> &left, Vector2<T> right) { return left = left + right; }
template <typename T>
constexpr Vector2<T> &operator-=(Vector2<T> &left, Vector2<T> right) { return left = left - right; }
template <typename T>
constexpr Vector2<T> &operator*=(Vector2<T> &left, Vector2<T> right) { return left = left * right; }
template <typename T>
constexpr Vector2<T> &operator/=(Vector2<T> &left, Vector2<T> right) { return left = left / right; }

template <typename T>
constexpr Vector2<T> &operator+=(Vector2<T> &left, Non_Deduced<T> right) { return left = left + right; }
template <typename T>
constexpr Vector2<T> &operator-=(Vector2<T> &left, Non_Deduced<T> right) { return left = left - right; }
template <typename T>
constexpr Vector2<T> &operator*=(Vector2<T> &left, Non_Deduced<T> right) { return left = left * right; }
template <typename T>
constexpr Vector2<T> &operator/=(Vector2<T> &left, Non_Deduced<T> right) { return left = left / right; }