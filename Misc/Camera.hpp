#pragma once

#include "../Base/Math.hpp"
#include "../Win32Stuff/WindowRelated.hpp"

inline constexpr f32 CAMERA_MOVE_SPEED = 3;
inline constexpr f32 CAMERA_LOOK_SPEED = 0.003f;
inline constexpr f32 CAMERA_PITCH_LIMIT = RadiansFromDegreesF32(89);

struct Camera3D
{
  Vector3<f32> position;
  f32 yaw;
  f32 pitch;
};

[[nodiscard]]
inline Vector3<f32> Camera3DForward(const Camera3D *camera)
{
  return {cosf(camera->pitch) * sinf(camera->yaw),
          sinf(camera->pitch),
          -cosf(camera->pitch) * cosf(camera->yaw)};
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

inline void Camera3DUpdate(Camera3D *camera, const Win32_Input_State *input, f32 frameTime)
{
  static auto mouseLast = input->mousePosition;
  auto mouseDelta = input->mousePosition - mouseLast;
  mouseLast = input->mousePosition;

  if (Win32_ButtonDown(input, Mouse_Button_Right))
  {
    camera->yaw += mouseDelta.x * CAMERA_LOOK_SPEED;
    camera->pitch -= mouseDelta.y * CAMERA_LOOK_SPEED;
    camera->pitch = Clamp(camera->pitch, -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);
  }

  auto forward = Camera3DForward(camera);
  auto right = Camera3DRight(forward);
  Vector3<f32> direction = {};
  if (Win32_KeyDown(input, 'W'))
  {
    direction += forward;
  }
  if (Win32_KeyDown(input, 'S'))
  {
    direction -= forward;
  }
  if (Win32_KeyDown(input, 'D'))
  {
    direction += right;
  }
  if (Win32_KeyDown(input, 'A'))
  {
    direction -= right;
  }
  if (Win32_KeyDown(input, VK_SPACE))
  {
    direction.y += 1;
  }
  if (Win32_KeyDown(input, VK_CONTROL))
  {
    direction.y -= 1;
  }
  camera->position += direction * (CAMERA_MOVE_SPEED * frameTime);
}