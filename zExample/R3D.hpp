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

  auto fbSizeF32 = Vector2Cast<f32>(frameBuffer->size);
  i32 left = RoundHalfDownF32(Clamp(leftMost, 0.0f, fbSizeF32.x));
  i32 right = RoundHalfDownF32(Clamp(rightMost, 0.0f, fbSizeF32.x));
  i32 top = RoundHalfDownF32(Clamp(topMost, 0.0f, fbSizeF32.y));
  i32 bottom = RoundHalfDownF32(Clamp(bottomMost, 0.0f, fbSizeF32.y));

  auto pixels = frameBuffer->colorData.ptr;
  auto depths = frameBuffer->depthData.ptr;
  usize fbWidth = frameBuffer->size.width;
  auto oneOverAreaTimes2 = 1 / areaTimes2; // dont needed when -ffast-math
  Vector3<f32> zVec = {p0.z, p1.z, p2.z};

  for (i32 y = top; y < bottom; y += 1)
  {
    auto pixelRow = pixels + y * fbWidth;
    auto depthRow = depths + y * fbWidth;
    for (i32 x = left; x < right; x += 1)
    {
      auto center = Vector2Cast<f32>(Vector2<i32>{x, y}) + Vector2<f32>{0.5, 0.5};
      auto e0 = R_EdgeFunctionCCW(p1.xy, p2.xy, center);
      auto e1 = R_EdgeFunctionCCW(p2.xy, p0.xy, center);
      auto e2 = R_EdgeFunctionCCW(p0.xy, p1.xy, center);

      if (bool isInside = e0 >= 0 && e1 >= 0 && e2 >= 0; isInside)
      {
        Vector3<f32> w = Vector3<f32>{e0, e1, e2} * oneOverAreaTimes2;
        f32 depth = DotProduct(w, zVec);
        if (depth < depthRow[x])
        {
          depthRow[x] = depth;
          pixelRow[x] = color;
        }
      }
    }
  }
}

struct R3D_Triangle
{
  Vector3<f32> (&positions)[3];
  Vector2<f32> (&texcoords)[3];
};

inline void R3D_RasterTriangleCCW_PS1Style(R_FrameBuffer *frameBuffer, const Image *image, R3D_Triangle triangle)
{
  assert(image->format == Image_Format_BGRA8 && "Image must be decoded as Image_Format_BGRA8");

  auto p0 = triangle.positions[0];
  auto p1 = triangle.positions[1];
  auto p2 = triangle.positions[2];

  auto areaTimes2 = R_EdgeFunctionCCW(p0.xy, p1.xy, p2.xy);
  if (areaTimes2 <= 0)
  {
    return;
  }

  auto leftMost = Minimum3(p0.x, p1.x, p2.x);
  auto rightMost = Maximum3(p0.x, p1.x, p2.x);
  auto topMost = Minimum3(p0.y, p1.y, p2.y);
  auto bottomMost = Maximum3(p0.y, p1.y, p2.y);

  auto fbSizeF32 = Vector2Cast<f32>(frameBuffer->size);
  i32 left = RoundHalfDownF32(Clamp(leftMost, 0.0f, fbSizeF32.x));
  i32 right = RoundHalfDownF32(Clamp(rightMost, 0.0f, fbSizeF32.x));
  i32 top = RoundHalfDownF32(Clamp(topMost, 0.0f, fbSizeF32.y));
  i32 bottom = RoundHalfDownF32(Clamp(bottomMost, 0.0f, fbSizeF32.y));

  auto pixels = frameBuffer->colorData.ptr;
  auto depths = frameBuffer->depthData.ptr;
  usize fbWidth = frameBuffer->size.width;
  auto oneOverAreaTimes2 = 1.0f / areaTimes2;

  auto uv0 = triangle.texcoords[0];
  auto uv1 = triangle.texcoords[1];
  auto uv2 = triangle.texcoords[2];

  Vector3<f32> zVec = {p0.z, p1.z, p2.z};
  Vector3<f32> uVec = {uv0.x, uv1.x, uv2.x};
  Vector3<f32> vVec = {uv0.y, uv1.y, uv2.y};

  auto srcPixels = reinterpret_cast<const R_B8G8R8A8 *>(image->pixels.ptr);
  auto imgSize = Vector2Cast<i32>(image->size);

  for (i32 y = top; y < bottom; y += 1)
  {
    auto pixelRow = pixels + y * fbWidth;
    auto depthRow = depths + y * fbWidth;
    for (i32 x = left; x < right; x += 1)
    {
      auto center = Vector2Cast<f32>(Vector2<i32>{x, y}) + Vector2<f32>{0.5, 0.5};
      auto e0 = R_EdgeFunctionCCW(p1.xy, p2.xy, center);
      auto e1 = R_EdgeFunctionCCW(p2.xy, p0.xy, center);
      auto e2 = R_EdgeFunctionCCW(p0.xy, p1.xy, center);

      if (bool isInside = e0 >= 0 && e1 >= 0 && e2 >= 0; isInside)
      {
        Vector3<f32> w = Vector3<f32>{e0, e1, e2} * oneOverAreaTimes2;
        f32 depth = DotProduct(w, zVec);
        if (depth < depthRow[x])
        {
          depthRow[x] = depth;

          // PS1 Naive Lerp
          f32 u = DotProduct(w, uVec);
          f32 v = DotProduct(w, vVec);

          i32 srcX = Clamp(static_cast<i32>(FloorF32(u * imgSize.width)), 0, imgSize.width - 1);
          i32 srcY = Clamp(static_cast<i32>(FloorF32(v * imgSize.height)), 0, imgSize.height - 1);

          pixelRow[x] = srcPixels[srcY * imgSize.width + srcX];
        }
      }
    }
  }
}
