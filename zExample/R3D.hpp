#pragma once

#include "R.hpp"

struct R3D_Camera
{
  Vector3<f32> position;
  f32 yaw;
  f32 pitch;
};

[[nodiscard]]
inline Vector3<f32> R3D_CameraForward(const R3D_Camera *camera)
{
  return {cosf(camera->pitch) * sinf(camera->yaw),
          sinf(camera->pitch),
          -cosf(camera->pitch) * cosf(camera->yaw)};
}

[[nodiscard]]
inline Vector3<f32> R3D_CameraRight(Vector3<f32> forward)
{
  return NormalizeF32(CrossProduct(forward, {0, 1, 0}));
}

[[nodiscard]]
inline Matrix4x4 R3D_CameraViewMatrix(const R3D_Camera *camera)
{
  auto forward = R3D_CameraForward(camera);
  auto right = R3D_CameraRight(forward);
  auto up = CrossProduct(right, forward);
  auto back = forward * -1.0f;
  auto eye = camera->position;

  return {{{right.x, up.x, back.x, 0},
           {right.y, up.y, back.y, 0},
           {right.z, up.z, back.z, 0},
           {-DotProduct(right, eye), -DotProduct(up, eye), -DotProduct(back, eye), 1}}};
}

inline void R3D_RasterTriangleFlatCCW(R_FrameBuffer *frameBuffer,
                                      const Vector3<f32> (&positions)[3],
                                      R_B8G8R8A8 color)
{
  auto p0 = positions[0];
  auto p1 = positions[1];
  auto p2 = positions[2];

  auto areaTimes2 = R_EdgeFunctionCCW(p0.xy, p1.xy, p2.xy);
  if (areaTimes2 <= 0)
  {
    return;
  }

  auto leftMost = Minimum3(p0.x, p1.x, p2.x);
  auto rightMost = Maximum3(p0.x, p1.x, p2.x);
  auto topMost = Minimum3(p0.y, p1.y, p2.y);
  auto bottomMost = Maximum3(p0.y, p1.y, p2.y);

  auto frameBufferSize2F32 = R_PointFrom2I32(frameBuffer->size);
  i32 left = RoundHalfDownF32(Clamp(leftMost, 0.0f, frameBufferSize2F32.x));
  i32 right = RoundHalfDownF32(Clamp(rightMost, 0.0f, frameBufferSize2F32.x));
  i32 top = RoundHalfDownF32(Clamp(topMost, 0.0f, frameBufferSize2F32.y));
  i32 bottom = RoundHalfDownF32(Clamp(bottomMost, 0.0f, frameBufferSize2F32.y));

  auto pixelBasePtr = frameBuffer->colorData.ptr;
  auto depthBasePtr = frameBuffer->depthData.ptr;
  usize width = frameBuffer->size.width;
  auto oneOverAreaTimes2 = 1 / areaTimes2; // dont needed when -ffast-math

  for (i32 y = top; y < bottom; y += 1)
  {
    auto pixelRow = pixelBasePtr + y * width;
    auto depthRow = depthBasePtr + y * width;
    for (i32 x = left; x < right; x += 1)
    {
      auto center = R_PointFrom2I32({x, y}) + Vector2<f32>{0.5, 0.5};
      auto w0 = R_EdgeFunctionCCW(p1.xy, p2.xy, center) * oneOverAreaTimes2;
      auto w1 = R_EdgeFunctionCCW(p2.xy, p0.xy, center) * oneOverAreaTimes2;
      auto w2 = R_EdgeFunctionCCW(p0.xy, p1.xy, center) * oneOverAreaTimes2;

      if (bool isInside = w0 >= 0 && w1 >= 0 && w2 >= 0; isInside)
      {
        f32 depth = w0 * p0.z + w1 * p1.z + w2 * p2.z;
        if (depth < depthRow[x])
        {
          depthRow[x] = depth;
          pixelRow[x] = color;
        }
      }
    }
  }
}