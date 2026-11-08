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

[[nodiscard]] constexpr f32
LerpF32(f32 start, f32 end, f32 t)
{
  return (1 - t) * start + t * end;
}

[[nodiscard]] constexpr f32
InverseLerpF32(f32 start, f32 end, f32 value)
{
  return (value - start) / (end - start);
}

inline constexpr f32 PI_F32 = 3.14159265358979323846f;
[[nodiscard]] constexpr f32 RadiansFromDegreesF32(f32 degrees) { return degrees * (PI_F32 / 180); }
[[nodiscard]] constexpr f32 DegreesFromRadiansF32(f32 radians) { return radians * (180 / PI_F32); }

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

[[nodiscard]] inline f32 LengthSquaredF32(Vector2<f32> vector) { return DotProduct(vector, vector); }
[[nodiscard]] inline f32 LengthF32(Vector2<f32> vector) { return SqrtF32(LengthSquaredF32(vector)); }
// a zero vector gives NaN
[[nodiscard]] inline Vector2<f32> NormalizeF32(Vector2<f32> vector) { return vector / LengthF32(vector); }

[[nodiscard]] constexpr Vector2<f32>
LerpF32(Vector2<f32> start, Vector2<f32> end, f32 t)
{
  return start * (1 - t) + end * t;
}

[[nodiscard]] constexpr Vector2<f32>
InverseLerpF32(Vector2<f32> start, Vector2<f32> end, Vector2<f32> value)
{
  return (value - start) / (end - start);
}

template <typename To, typename From>
[[nodiscard]] constexpr Vector2<To> Vector2Cast(Vector2<From> vector)
{
  return {static_cast<To>(vector.data[0]),
          static_cast<To>(vector.data[1])};
}

template <typename T>
union Rect
{
  static_assert(Is_Number<T>::VALUE, "Rect needs a numeric type");

  T data[4];

  struct
  {
    T x, y, width, height;
  };

  struct
  {
    Vector2<T> position;
    Vector2<T> size;
  };
};

// the right and bottom edges are outside
template <typename T>
[[nodiscard]] constexpr bool RectContains(Rect<T> rect, Non_Deduced<Vector2<T>> point)
{
  auto bottomRight = rect.position + rect.size;
  bool insideX = point.x >= rect.x && point.x < bottomRight.x;
  bool insideY = point.y >= rect.y && point.y < bottomRight.y;
  return insideX && insideY;
}

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

[[nodiscard]] inline f32 LengthSquaredF32(Vector3<f32> vector) { return DotProduct(vector, vector); }
[[nodiscard]] inline f32 LengthF32(Vector3<f32> vector) { return SqrtF32(LengthSquaredF32(vector)); }
// a zero vector gives NaN
[[nodiscard]] inline Vector3<f32> NormalizeF32(Vector3<f32> vector) { return vector / LengthF32(vector); }

[[nodiscard]] constexpr Vector3<f32>
LerpF32(Vector3<f32> start, Vector3<f32> end, f32 t)
{
  return start * (1 - t) + end * t;
}

[[nodiscard]] constexpr Vector3<f32>
InverseLerpF32(Vector3<f32> start, Vector3<f32> end, Vector3<f32> value)
{
  return (value - start) / (end - start);
}

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

[[nodiscard]] constexpr Vector4<f32>
LerpF32(Vector4<f32> start, Vector4<f32> end, f32 t)
{
  return start * (1 - t) + end * t;
}

union Matrix4x4
{
  Vector4<f32> columns[4];
  f32 data[16];
};

[[nodiscard]]
constexpr Vector4<f32> operator*(const Matrix4x4 &matrix, Vector4<f32> vector)
{
  return matrix.columns[0] * vector.data[0] +
         matrix.columns[1] * vector.data[1] +
         matrix.columns[2] * vector.data[2] +
         matrix.columns[3] * vector.data[3];
}

[[nodiscard]]
constexpr Matrix4x4 operator*(const Matrix4x4 &left, const Matrix4x4 &right)
{
  return {left * right.columns[0],
          left * right.columns[1],
          left * right.columns[2],
          left * right.columns[3]};
}

// D3D depth, top is -1
[[nodiscard]]
constexpr Matrix4x4 Matrix4x4OrthoRH(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ)
{
  return {{{2 / (right - left), 0, 0, 0},
           {0, 2 / (bottom - top), 0, 0},
           {0, 0, 1 / (nearZ - farZ), 0},
           {-(right + left) / (right - left), (top + bottom) / (top - bottom), nearZ / (nearZ - farZ), 1}}};
}

// D3D depth, top is -1
[[nodiscard]]
inline Matrix4x4 Matrix4x4PerspectiveRH(f32 fovYRadians, f32 aspect, f32 nearZ, f32 farZ)
{
  f32 focal = 1 / tanf(fovYRadians / 2);
  return {{{focal / aspect, 0, 0, 0},
           {0, -focal, 0, 0},
           {0, 0, farZ / (nearZ - farZ), -1},
           {0, 0, nearZ * farZ / (nearZ - farZ), 0}}};
}

[[nodiscard]]
constexpr Matrix4x4 Matrix4x4Identity()
{
  return {{{1, 0, 0, 0},
           {0, 1, 0, 0},
           {0, 0, 1, 0},
           {0, 0, 0, 1}}};
}

[[nodiscard]]
constexpr Matrix4x4 Matrix4x4Translate(Vector3<f32> offset)
{
  return {{{1, 0, 0, 0},
           {0, 1, 0, 0},
           {0, 0, 1, 0},
           {offset.data[0], offset.data[1], offset.data[2], 1}}};
}

[[nodiscard]]
constexpr Matrix4x4 Matrix4x4Scale(Vector3<f32> scale)
{
  return {{{scale.data[0], 0, 0, 0},
           {0, scale.data[1], 0, 0},
           {0, 0, scale.data[2], 0},
           {0, 0, 0, 1}}};
}

// x' = x + xByY * y + xByZ * z
[[nodiscard]]
constexpr Matrix4x4 Matrix4x4Shear(f32 xByY, f32 xByZ, f32 yByX, f32 yByZ, f32 zByX, f32 zByY)
{
  return {{{1, yByX, zByX, 0},
           {xByY, 1, zByY, 0},
           {xByZ, yByZ, 1, 0},
           {0, 0, 0, 1}}};
}

// rotations are counter-clockwise when the axis points at the viewer
[[nodiscard]]
inline Matrix4x4 Matrix4x4RotationX(f32 radians)
{
  f32 c = cosf(radians);
  f32 s = sinf(radians);
  return {{{1, 0, 0, 0},
           {0, c, s, 0},
           {0, -s, c, 0},
           {0, 0, 0, 1}}};
}

[[nodiscard]]
inline Matrix4x4 Matrix4x4RotationY(f32 radians)
{
  f32 c = cosf(radians);
  f32 s = sinf(radians);
  return {{{c, 0, -s, 0},
           {0, 1, 0, 0},
           {s, 0, c, 0},
           {0, 0, 0, 1}}};
}

[[nodiscard]]
inline Matrix4x4 Matrix4x4RotationZ(f32 radians)
{
  f32 c = cosf(radians);
  f32 s = sinf(radians);
  return {{{c, s, 0, 0},
           {-s, c, 0, 0},
           {0, 0, 1, 0},
           {0, 0, 0, 1}}};
}

// axis must be normalized
[[nodiscard]]
inline Matrix4x4 Matrix4x4RotationAxis(Vector3<f32> axis, f32 radians)
{
  f32 c = cosf(radians);
  f32 s = sinf(radians);
  f32 t = 1 - c;
  f32 x = axis.x;
  f32 y = axis.y;
  f32 z = axis.z;
  return {{{t * x * x + c, t * x * y + s * z, t * x * z - s * y, 0},
           {t * x * y - s * z, t * y * y + c, t * y * z + s * x, 0},
           {t * x * z + s * y, t * y * z - s * x, t * z * z + c, 0},
           {0, 0, 0, 1}}};
}