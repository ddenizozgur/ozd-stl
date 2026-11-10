#pragma once

#include "../Base/Math.hpp"

struct Camera3D
{
  Vector3<f32> position;
  f32 yaw;
  f32 pitch;
};

[[nodiscard]]
inline Vector3<f32> Camera3DForward(const Camera3D *camera)
{
  return DirectionFromYawPitch(camera->yaw, camera->pitch);
}

[[nodiscard]]
inline Vector3<f32> Camera3DRight(Vector3<f32> forward)
{
  return NormalizeF32(CrossProduct(forward, {0, 1, 0}));
}

[[nodiscard]]
inline Matrix4x4 Camera3DViewMatrix(const Camera3D *camera)
{
  auto forward = Camera3DForward(camera);
  auto right = Camera3DRight(forward);
  auto up = CrossProduct(right, forward);
  auto back = forward * -1.0f;
  auto eye = camera->position;

  return {{{right.x, up.x, back.x, 0},
           {right.y, up.y, back.y, 0},
           {right.z, up.z, back.z, 0},
           {-DotProduct(right, eye), -DotProduct(up, eye), -DotProduct(back, eye), 1}}};
}