#pragma once

#include "Core.hpp"
#include <math.h>

// disassembly with clang -O2 -msse4.2
[[nodiscard]] inline f32 FloorF32(f32 x) { return floorf(x); }               // roundss
[[nodiscard]] inline f32 CeilF32(f32 x) { return ceilf(x); }                 // roundss
[[nodiscard]] inline f32 RoundHalfDownF32(f32 x) { return ceilf(x - 0.5f); } // addss, roundss. msvc uses subss
[[nodiscard]] inline f32 RoundHalfUpF32(f32 x) { return floorf(x + 0.5f); }  // addss, roundss
[[nodiscard]] inline f32 SqrtF32(f32 x) { return sqrtf(x); }                 // ucomiss, sqrtss. negative x calls sqrtf to set errno
[[nodiscard]] inline f32 RoundF32(f32 x) { return roundf(x); }               // andps, orps, addss, roundss. msvc calls roundf

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
  constexpr Vector2(T first, T second) : data{first, second} {}
};

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator+(Vector2<T> left, Vector2<T> right)
{
  T x = left.data[0] + right.data[0];
  T y = left.data[1] + right.data[1];
  return {x, y};
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator-(Vector2<T> left, Vector2<T> right)
{
  T x = left.data[0] - right.data[0];
  T y = left.data[1] - right.data[1];
  return {x, y};
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(Vector2<T> left, Vector2<T> right)
{
  T x = left.data[0] * right.data[0];
  T y = left.data[1] * right.data[1];
  return {x, y};
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator/(Vector2<T> left, Vector2<T> right)
{
  T x = left.data[0] / right.data[0];
  T y = left.data[1] / right.data[1];
  return {x, y};
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator+(Vector2<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] + right;
  T y = left.data[1] + right;
  return {x, y};
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator-(Vector2<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] - right;
  T y = left.data[1] - right;
  return {x, y};
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator*(Vector2<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] * right;
  T y = left.data[1] * right;
  return {x, y};
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator/(Vector2<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] / right;
  T y = left.data[1] / right;
  return {x, y};
}

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

template <typename T>
[[nodiscard]] constexpr T DotProduct(Vector2<T> left, Vector2<T> right)
{
  return left.data[0] * right.data[0] +
         left.data[1] * right.data[1];
}

[[nodiscard]] inline f32 LengthSquared(Vector2<f32> vector) { return DotProduct(vector, vector); }
[[nodiscard]] inline f32 Length(Vector2<f32> vector) { return SqrtF32(LengthSquared(vector)); }
// a zero vector gives NaN
[[nodiscard]] inline Vector2<f32> Normalize(Vector2<f32> vector) { return vector / Length(vector); }

template <typename T>
union Vector3
{
  static_assert(Is_Number<T>::VALUE, "Vector3 needs a numeric type");

  T data[3];

  struct
  {
    T x;
    union
    {
      struct
      {
        T y, z;
      };
      Vector2<T> yz;
    };
  };

  struct
  {
    Vector2<T> xy;
    T z_;
  };

  struct
  {
    T width, height, depth;
  };

  // only exist for -Wmissing-braces
  Vector3() = default;
  constexpr Vector3(T first, T second, T third) : data{first, second, third} {}
};

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator+(Vector3<T> left, Vector3<T> right)
{
  T x = left.data[0] + right.data[0];
  T y = left.data[1] + right.data[1];
  T z = left.data[2] + right.data[2];
  return {x, y, z};
}

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator-(Vector3<T> left, Vector3<T> right)
{
  T x = left.data[0] - right.data[0];
  T y = left.data[1] - right.data[1];
  T z = left.data[2] - right.data[2];
  return {x, y, z};
}

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator*(Vector3<T> left, Vector3<T> right)
{
  T x = left.data[0] * right.data[0];
  T y = left.data[1] * right.data[1];
  T z = left.data[2] * right.data[2];
  return {x, y, z};
}

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator/(Vector3<T> left, Vector3<T> right)
{
  T x = left.data[0] / right.data[0];
  T y = left.data[1] / right.data[1];
  T z = left.data[2] / right.data[2];
  return {x, y, z};
}

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator+(Vector3<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] + right;
  T y = left.data[1] + right;
  T z = left.data[2] + right;
  return {x, y, z};
}

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator-(Vector3<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] - right;
  T y = left.data[1] - right;
  T z = left.data[2] - right;
  return {x, y, z};
}

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator*(Vector3<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] * right;
  T y = left.data[1] * right;
  T z = left.data[2] * right;
  return {x, y, z};
}

template <typename T>
[[nodiscard]] constexpr Vector3<T> operator/(Vector3<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] / right;
  T y = left.data[1] / right;
  T z = left.data[2] / right;
  return {x, y, z};
}

template <typename T>
constexpr Vector3<T> &operator+=(Vector3<T> &left, Vector3<T> right) { return left = left + right; }
template <typename T>
constexpr Vector3<T> &operator-=(Vector3<T> &left, Vector3<T> right) { return left = left - right; }
template <typename T>
constexpr Vector3<T> &operator*=(Vector3<T> &left, Vector3<T> right) { return left = left * right; }
template <typename T>
constexpr Vector3<T> &operator/=(Vector3<T> &left, Vector3<T> right) { return left = left / right; }

template <typename T>
constexpr Vector3<T> &operator+=(Vector3<T> &left, Non_Deduced<T> right) { return left = left + right; }
template <typename T>
constexpr Vector3<T> &operator-=(Vector3<T> &left, Non_Deduced<T> right) { return left = left - right; }
template <typename T>
constexpr Vector3<T> &operator*=(Vector3<T> &left, Non_Deduced<T> right) { return left = left * right; }
template <typename T>
constexpr Vector3<T> &operator/=(Vector3<T> &left, Non_Deduced<T> right) { return left = left / right; }

template <typename T>
[[nodiscard]] constexpr T DotProduct(Vector3<T> left, Vector3<T> right)
{
  return left.data[0] * right.data[0] +
         left.data[1] * right.data[1] +
         left.data[2] * right.data[2];
}

template <typename T>
[[nodiscard]] constexpr Vector3<T> CrossProduct(Vector3<T> left, Vector3<T> right)
{
  T x = left.data[1] * right.data[2] - left.data[2] * right.data[1];
  T y = left.data[2] * right.data[0] - left.data[0] * right.data[2];
  T z = left.data[0] * right.data[1] - left.data[1] * right.data[0];
  return {x, y, z};
}

[[nodiscard]] inline f32 LengthSquared(Vector3<f32> vector) { return DotProduct(vector, vector); }
[[nodiscard]] inline f32 Length(Vector3<f32> vector) { return SqrtF32(LengthSquared(vector)); }
// a zero vector gives NaN
[[nodiscard]] inline Vector3<f32> Normalize(Vector3<f32> vector) { return vector / Length(vector); }

template <typename T>
union Vector4
{
  static_assert(Is_Number<T>::VALUE, "Vector4 needs a numeric type");

  T data[4];

  struct
  {
    union
    {
      struct
      {
        T x, y, z;
      };
      Vector3<T> xyz;
    };
    T w;
  };

  struct
  {
    Vector2<T> xy;
    Vector2<T> zw;
  };

  struct
  {
    T x_;
    union
    {
      Vector2<T> yz;
      Vector3<T> yzw;
    };
  };

  // only exist for -Wmissing-braces
  Vector4() = default;
  constexpr Vector4(T first, T second, T third, T fourth) : data{first, second, third, fourth} {}
};

template <typename T>
[[nodiscard]] constexpr Vector4<T> operator+(Vector4<T> left, Vector4<T> right)
{
  T x = left.data[0] + right.data[0];
  T y = left.data[1] + right.data[1];
  T z = left.data[2] + right.data[2];
  T w = left.data[3] + right.data[3];
  return {x, y, z, w};
}

template <typename T>
[[nodiscard]] constexpr Vector4<T> operator-(Vector4<T> left, Vector4<T> right)
{
  T x = left.data[0] - right.data[0];
  T y = left.data[1] - right.data[1];
  T z = left.data[2] - right.data[2];
  T w = left.data[3] - right.data[3];
  return {x, y, z, w};
}

template <typename T>
[[nodiscard]] constexpr Vector4<T> operator*(Vector4<T> left, Vector4<T> right)
{
  T x = left.data[0] * right.data[0];
  T y = left.data[1] * right.data[1];
  T z = left.data[2] * right.data[2];
  T w = left.data[3] * right.data[3];
  return {x, y, z, w};
}

template <typename T>
[[nodiscard]] constexpr Vector4<T> operator/(Vector4<T> left, Vector4<T> right)
{
  T x = left.data[0] / right.data[0];
  T y = left.data[1] / right.data[1];
  T z = left.data[2] / right.data[2];
  T w = left.data[3] / right.data[3];
  return {x, y, z, w};
}

template <typename T>
[[nodiscard]] constexpr Vector4<T> operator+(Vector4<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] + right;
  T y = left.data[1] + right;
  T z = left.data[2] + right;
  T w = left.data[3] + right;
  return {x, y, z, w};
}

template <typename T>
[[nodiscard]] constexpr Vector4<T> operator-(Vector4<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] - right;
  T y = left.data[1] - right;
  T z = left.data[2] - right;
  T w = left.data[3] - right;
  return {x, y, z, w};
}

template <typename T>
[[nodiscard]] constexpr Vector4<T> operator*(Vector4<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] * right;
  T y = left.data[1] * right;
  T z = left.data[2] * right;
  T w = left.data[3] * right;
  return {x, y, z, w};
}

template <typename T>
[[nodiscard]] constexpr Vector4<T> operator/(Vector4<T> left, Non_Deduced<T> right)
{
  T x = left.data[0] / right;
  T y = left.data[1] / right;
  T z = left.data[2] / right;
  T w = left.data[3] / right;
  return {x, y, z, w};
}

template <typename T>
constexpr Vector4<T> &operator+=(Vector4<T> &left, Vector4<T> right) { return left = left + right; }
template <typename T>
constexpr Vector4<T> &operator-=(Vector4<T> &left, Vector4<T> right) { return left = left - right; }
template <typename T>
constexpr Vector4<T> &operator*=(Vector4<T> &left, Vector4<T> right) { return left = left * right; }
template <typename T>
constexpr Vector4<T> &operator/=(Vector4<T> &left, Vector4<T> right) { return left = left / right; }

template <typename T>
constexpr Vector4<T> &operator+=(Vector4<T> &left, Non_Deduced<T> right) { return left = left + right; }
template <typename T>
constexpr Vector4<T> &operator-=(Vector4<T> &left, Non_Deduced<T> right) { return left = left - right; }
template <typename T>
constexpr Vector4<T> &operator*=(Vector4<T> &left, Non_Deduced<T> right) { return left = left * right; }
template <typename T>
constexpr Vector4<T> &operator/=(Vector4<T> &left, Non_Deduced<T> right) { return left = left / right; }

template <typename T>
[[nodiscard]] constexpr T DotProduct(Vector4<T> left, Vector4<T> right)
{
  return left.data[0] * right.data[0] +
         left.data[1] * right.data[1] +
         left.data[2] * right.data[2] +
         left.data[3] * right.data[3];
}
