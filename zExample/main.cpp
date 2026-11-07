#include "../Base/Time.hpp"
#include "../Win32Stuff/TaskDialog.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WindowRelated.hpp"
#include "R3D.hpp"

struct Example_Globals
{
  Win32_Window_State windowState;
  Win32_Input_State inputState;
};
inline Example_Globals g;

inline LRESULT CALLBACK ExampleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  Win32_WindowStateOnMessage(&g.windowState, hWnd, message, wParam, lParam);
  Win32_InputStateOnMessage(&g.inputState, hWnd, message, wParam, lParam);
  return DefWindowProcW(hWnd, message, wParam, lParam);
}

void UpdateTitle(HWND hWnd, f32 frameTime)
{
  static f32 elapsedTime = 0;
  static u32 frameCounter = 0;

  if (elapsedTime >= 1)
  {
    auto scratch = ScratchBegin();
    Defer(ScratchEnd(scratch););

    auto title = StringFmt(scratch.arena, "SomeApp | %u FPS | %.4f ms",
                           frameCounter, frameTime * Thousand(1));
    Win32_SetWindowText(hWnd, title);

    elapsedTime -= 1;
    frameCounter = 0;
  }

  elapsedTime += frameTime;
  frameCounter += 1;
}

inline constexpr Vector3<f32> CUBE[36] = {
  // +z
  {-0.5f, 0.5f, 0.5f},
  {0.5f, 0.5f, 0.5f},
  {0.5f, -0.5f, 0.5f},
  {-0.5f, 0.5f, 0.5f},
  {0.5f, -0.5f, 0.5f},
  {-0.5f, -0.5f, 0.5f},
  // -z
  {0.5f, 0.5f, -0.5f},
  {-0.5f, 0.5f, -0.5f},
  {-0.5f, -0.5f, -0.5f},
  {0.5f, 0.5f, -0.5f},
  {-0.5f, -0.5f, -0.5f},
  {0.5f, -0.5f, -0.5f},
  // -x
  {-0.5f, 0.5f, -0.5f},
  {-0.5f, 0.5f, 0.5f},
  {-0.5f, -0.5f, 0.5f},
  {-0.5f, 0.5f, -0.5f},
  {-0.5f, -0.5f, 0.5f},
  {-0.5f, -0.5f, -0.5f},
  // +x
  {0.5f, 0.5f, 0.5f},
  {0.5f, 0.5f, -0.5f},
  {0.5f, -0.5f, -0.5f},
  {0.5f, 0.5f, 0.5f},
  {0.5f, -0.5f, -0.5f},
  {0.5f, -0.5f, 0.5f},
  // +y
  {-0.5f, 0.5f, -0.5f},
  {0.5f, 0.5f, -0.5f},
  {0.5f, 0.5f, 0.5f},
  {-0.5f, 0.5f, -0.5f},
  {0.5f, 0.5f, 0.5f},
  {-0.5f, 0.5f, 0.5f},
  // -y
  {-0.5f, -0.5f, 0.5f},
  {0.5f, -0.5f, 0.5f},
  {0.5f, -0.5f, -0.5f},
  {-0.5f, -0.5f, 0.5f},
  {0.5f, -0.5f, -0.5f},
  {-0.5f, -0.5f, -0.5f},
};

inline constexpr f32 WORLD_NEAR_Z = 0.1f;
inline constexpr f32 WORLD_FAR_Z = 100;
inline constexpr f32 CAMERA_MOVE_SPEED = 3;
inline constexpr f32 CAMERA_LOOK_SPEED = 0.003f;
inline constexpr f32 CAMERA_PITCH_LIMIT = RadiansFromDegreesF32(89);

[[nodiscard]]
inline Matrix4x4 WorldPerspectiveMatrix(f32 fovYRadians, f32 aspect)
{
  return Matrix4x4PerspectiveRH(fovYRadians, aspect, WORLD_NEAR_Z, WORLD_FAR_Z);
}

inline void CameraUpdate(R3D_Camera *camera, f32 frameTime)
{
  auto input = &g.inputState;

  static auto mouseLast = input->mousePosition;
  auto mouseDelta = input->mousePosition - mouseLast;
  mouseLast = input->mousePosition;

  if (Win32_ButtonDown(input, Mouse_Button_Right))
  {
    camera->yaw += mouseDelta.x * CAMERA_LOOK_SPEED;
    camera->pitch -= mouseDelta.y * CAMERA_LOOK_SPEED;
    camera->pitch = Clamp(camera->pitch, -CAMERA_PITCH_LIMIT, CAMERA_PITCH_LIMIT);
  }

  auto forward = R3D_CameraForward(camera);
  auto right = R3D_CameraRight(forward);
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

[[nodiscard]]
inline Pair<Vector2<f32>, bool> ProjectToScreen(const R_FrameBuffer *frameBuffer,
                                                const Matrix4x4 &toClip, Vector3<f32> position)
{
  Vector4<f32> point = {.w = 1};
  point.xyz = position;

  auto clip = toClip * point;
  if (clip.w < WORLD_NEAR_Z)
  {
    return {};
  }
  return {R_NDCToPosition(clip.xy / clip.w, {{}, frameBuffer->size}), true};
}

inline void DrawModelTriangleFlat(R_FrameBuffer *frameBuffer, const Matrix4x4 &mvp,
                                  const Vector3<f32> (&positions)[3], R_B8G8R8A8 color)
{
  auto [screen0, ok0] = ProjectToScreen(frameBuffer, mvp, positions[0]);
  auto [screen1, ok1] = ProjectToScreen(frameBuffer, mvp, positions[1]);
  auto [screen2, ok2] = ProjectToScreen(frameBuffer, mvp, positions[2]);
  if (ok0 && ok1 && ok2)
  {
    R_RasterTriangleFlat(frameBuffer, {screen0, screen1, screen2}, color);
  }
}

inline void DrawWorldRect(R_FrameBuffer *frameBuffer, const Matrix4x4 &viewProjection,
                          Vector3<f32> world, Vector2<i32> size, R_B8G8R8A8 color)
{
  if (auto [screen, ok] = ProjectToScreen(frameBuffer, viewProjection, world); ok)
  {
    R_FillRect(frameBuffer, R_PointFrom2F32(screen) - size / 2, size, color);
  }
}

inline void DrawWorldText(R_FrameBuffer *frameBuffer, const Matrix4x4 &viewProjection, R_BitmapFont font,
                          Vector3<f32> world, R_B8G8R8A8 color, String text)
{
  if (auto [screen, ok] = ProjectToScreen(frameBuffer, viewProjection, world); ok)
  {
    R_PutStringAscii(frameBuffer, font, R_PointFrom2F32(screen), color, text);
  }
}

inline void DrawWorldLine(R_FrameBuffer *frameBuffer, const Matrix4x4 &viewProjection,
                          Vector3<f32> a, Vector3<f32> b, R_B8G8R8A8 color)
{
  Vector4<f32> pointA = {.w = 1};
  pointA.xyz = a;
  Vector4<f32> pointB = {.w = 1};
  pointB.xyz = b;

  auto clipA = viewProjection * pointA;
  auto clipB = viewProjection * pointB;
  if (clipA.w < WORLD_NEAR_Z && clipB.w < WORLD_NEAR_Z)
  {
    return;
  }

  auto nearFraction = (WORLD_NEAR_Z - clipA.w) / (clipB.w - clipA.w);
  auto clipOnNear = LerpF32(clipA, clipB, nearFraction);
  if (clipA.w < WORLD_NEAR_Z)
  {
    clipA = clipOnNear;
  }
  else if (clipB.w < WORLD_NEAR_Z)
  {
    clipB = clipOnNear;
  }

  R_RasterLine(frameBuffer,
               R_NDCToPosition(clipA.xy / clipA.w, {{}, frameBuffer->size}),
               R_NDCToPosition(clipB.xy / clipB.w, {{}, frameBuffer->size}),
               color);
}

inline void DrawWorldAxes(R_FrameBuffer *frameBuffer, const Matrix4x4 &viewProjection, R_BitmapFont font)
{
  struct Axis_Label
  {
    Vector3<f32> position;
    R_B8G8R8A8 color;
    String text;
  };
  Axis_Label labels[] = {
    {{0, 0, 0}, R_CGA_WHITE, StringLit("0,0,0")},
    {{1, 0, 0}, R_CGA_LIGHT_RED, StringLit("+x")},
    {{0, 1, 0}, R_CGA_LIGHT_GREEN, StringLit("+y")},
    {{0, 0, 1}, R_CGA_LIGHT_CYAN, StringLit("+z")},
  };
  for (auto &label : labels)
  {
    DrawWorldLine(frameBuffer, viewProjection, {0, 0, 0}, label.position, label.color);
    DrawWorldRect(frameBuffer, viewProjection, label.position, {4, 4}, label.color);
    DrawWorldText(frameBuffer, viewProjection, font, label.position, label.color, label.text);
  }
}

inline void DrawCube(R_FrameBuffer *frameBuffer, const Matrix4x4 &mvp)
{
  for (usize i = 0; i < ArrayCount(CUBE); i += 3)
  {
    auto color = R_PALETTE_VINIK24[(i / 3) % ArrayCount(R_PALETTE_VINIK24)];
    DrawModelTriangleFlat(frameBuffer, mvp, {CUBE[i + 0], CUBE[i + 1], CUBE[i + 2]}, color);
    // DrawWorldLine(frameBuffer, mvp, CUBE[i + 0], CUBE[i + 1], R_WHITE);
    // DrawWorldLine(frameBuffer, mvp, CUBE[i + 1], CUBE[i + 2], R_WHITE);
    // DrawWorldLine(frameBuffer, mvp, CUBE[i + 2], CUBE[i + 0], R_WHITE);
  }
}

int main()
{
  Vector2<i32> windowSize = {1600, 900};
  auto [classId, classOk] = Win32_RegisterWindowClass(StringLit("ExampleClass"), ExampleWndProc);
  auto hWnd = Win32_CreateWindow(classId, StringLit("SomeApp"), windowSize, Window_Kind_Normal);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic, true);

  auto arena = ArenaInit();
  auto frameBuffer = R_CreateFrameBuffer(&arena, windowSize / 2);
  ShowWindow(hWnd, SW_SHOWMAXIMIZED);

  auto timeNow = Win32_NowNanoseconds();
  auto timeLast = timeNow;

  R3D_Camera camera = {.position = {1, 1, 4}};
  f32 cubeAngle = 0;

  for (; !g.windowState.destroyed;)
  {
    timeNow = Win32_NowNanoseconds();
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    Win32_PollMessages(&g.inputState);

    CameraUpdate(&camera, frameTime);
    cubeAngle += frameTime;

    {
      auto hDC = GetDC(hWnd);
      Defer(ReleaseDC(hWnd, hDC););

      R_Clear(&frameBuffer, {0x44, 0x33, 0x22, 0xff});
      Defer(R_PresentFrameBuffer(hWnd, hDC, frameBuffer););

      // auto viewport = R_PresentViewport(g.windowState.clientSize, frameBuffer.size);
      // auto mouseNDC = R_PositionToNDC(R_PointFrom2I32(g.inputState.mousePosition), viewport);
      // auto mouse2F32 = R_NDCToPosition(mouseNDC, {{}, frameBuffer.size});

      auto frameBufferSize2F32 = R_PointFrom2I32(frameBuffer.size);
      auto aspectRatio = frameBufferSize2F32.width / frameBufferSize2F32.height;
      auto projectionMatrix = WorldPerspectiveMatrix(RadiansFromDegreesF32(60), aspectRatio);
      auto viewProjectionMatrix = projectionMatrix * R3D_CameraViewMatrix(&camera);
      auto modelMatrix = Matrix4x4Translate({0, 1, 0}) *
                         Matrix4x4RotationY(cubeAngle) *
                         Matrix4x4Shear(0.5f, 0, 0, 0, 0, 0) *
                         Matrix4x4Scale({2, 2, 2});

      DrawCube(&frameBuffer, viewProjectionMatrix * modelMatrix);
      DrawWorldAxes(&frameBuffer, viewProjectionMatrix, R_FONT_3DFX);
    }

    // DwmFlush();
  }
}

inline void RasterTriangleAscii(R_FrameBuffer *frameBuffer, R_BitmapFont font,
                                const Vector2<f32> (&positions)[3], R_B8G8R8A8 color, u8 c)
{
  auto p0 = positions[0];
  auto p1 = positions[1];
  auto p2 = positions[2];

  i32 leftMost = Minimum3(p0.x, p1.x, p2.x);
  i32 rightMost = Maximum3(p0.x, p1.x, p2.x);
  i32 topMost = Minimum3(p0.y, p1.y, p2.y);
  i32 bottomMost = Maximum3(p0.y, p1.y, p2.y);

  leftMost -= leftMost % font.glyphSize.width;
  topMost -= topMost % font.glyphSize.height;

  for (i32 y = topMost; y < bottomMost; y += font.glyphSize.height)
  {
    for (i32 x = leftMost; x < rightMost; x += font.glyphSize.width)
    {
      auto center = R_PointFrom2I32({x, y}) + R_PointFrom2I32(font.glyphSize) * 0.5f;
      bool isIn = R_EdgeFunction(p0, p1, center) >= 0 &&
                  R_EdgeFunction(p1, p2, center) >= 0 &&
                  R_EdgeFunction(p2, p0, center) >= 0;
      if (isIn)
      {
        R_PutChar(frameBuffer, font, {x, y}, color, c);
      }
    }
  }
}
