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
