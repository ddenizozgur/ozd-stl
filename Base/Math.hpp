#pragma once

#include "Core.hpp"

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

  // these only exist for -Wmissing-braces
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