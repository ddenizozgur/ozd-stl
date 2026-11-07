#define _CRT_SECURE_NO_WARNINGS

#include "../Base/File.hpp"
#include "../Base/Time.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WindowRelated.hpp"
#include "R3D.hpp"

#define CGLTF_IMPLEMENTATION
#include "../Vendor/cgltf.h"

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

struct Mesh
{
  Slice<Vector3<f32>> positions;
  Slice<u32> indices;
};

inline Mesh LoadMesh(Arena_Allocator *arena, const char *path)
{
  cgltf_options options = {};
  cgltf_data *data = nullptr;
  if (cgltf_parse_file(&options, path, &data) != cgltf_result_success)
  {
    return {};
  }
  Defer(cgltf_free(data););

  if (cgltf_load_buffers(&options, data, path) != cgltf_result_success ||
      cgltf_validate(data) != cgltf_result_success)
  {
    return {};
  }

  auto primitive = &data->meshes[0].primitives[0];
  auto accessor = cgltf_find_accessor(primitive, cgltf_attribute_type_position, 0);

  Mesh mesh = {};
  mesh.positions = ArenaPushSlice<Vector3<f32>>(arena, accessor->count);
  cgltf_accessor_unpack_floats(accessor, mesh.positions.ptr->data, mesh.positions.count * 3);
  mesh.indices = ArenaPushSlice<u32>(arena, primitive->indices->count);
  cgltf_accessor_unpack_indices(primitive->indices, mesh.indices.ptr, sizeof(u32), mesh.indices.count);
  return mesh;
}

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
inline Pair<Vector3<f32>, bool> ProjectToScreen(const R_FrameBuffer *frameBuffer,
                                                const Matrix4x4 &toClip, Vector3<f32> position)
{
  Vector4<f32> point = {.w = 1};
  point.xyz = position;

  auto clip = toClip * point;
  if (clip.w < WORLD_NEAR_Z)
  {
    return {};
  }
  Vector2<f32> screen = R_NDCToPosition(clip.xy / clip.w, {{}, frameBuffer->size});
  f32 depth = clip.z / clip.w;
  return {{screen.x, screen.y, depth}, true};
}

inline void DrawTriangleFlat(R_FrameBuffer *frameBuffer, const Matrix4x4 &mvp,
                             const Vector3<f32> (&positions)[3], R_B8G8R8A8 color)
{
  auto [screen0, ok0] = ProjectToScreen(frameBuffer, mvp, positions[0]);
  auto [screen1, ok1] = ProjectToScreen(frameBuffer, mvp, positions[1]);
  auto [screen2, ok2] = ProjectToScreen(frameBuffer, mvp, positions[2]);
  if (ok0 && ok1 && ok2)
  {
    R3D_RasterTriangleFlatCCW(frameBuffer, {screen0, screen1, screen2}, color);
  }
}

inline void DrawWorldRect(R_FrameBuffer *frameBuffer, const Matrix4x4 &viewProjection,
                          Vector3<f32> world, Vector2<i32> size, R_B8G8R8A8 color)
{
  if (auto [screen, ok] = ProjectToScreen(frameBuffer, viewProjection, world); ok)
  {
    R_FillRect(frameBuffer, R_PointFrom2F32(screen.xy) - size / 2, size, color);
  }
}

inline void DrawWorldText(R_FrameBuffer *frameBuffer, const Matrix4x4 &viewProjection, R_BitmapFont font,
                          Vector3<f32> world, R_B8G8R8A8 color, String text)
{
  if (auto [screen, ok] = ProjectToScreen(frameBuffer, viewProjection, world); ok)
  {
    R_PutStringAscii(frameBuffer, font, R_PointFrom2F32(screen.xy), color, text);
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

inline void DrawMesh(R_FrameBuffer *frameBuffer, const Matrix4x4 &mvp, Mesh mesh)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto projected = ArenaPushSlice<Pair<Vector3<f32>, bool>>(scratch.arena, mesh.positions.count);
  for (usize v = 0; v < mesh.positions.count; v += 1)
  {
    projected[v] = ProjectToScreen(frameBuffer, mvp, mesh.positions[v]);
  }

  for (usize i = 0; i < mesh.indices.count; i += 3)
  {
    auto [a, aOk] = projected[mesh.indices[i + 0]];
    auto [b, bOk] = projected[mesh.indices[i + 1]];
    auto [c, cOk] = projected[mesh.indices[i + 2]];
    if (aOk && bOk && cOk)
    {
      auto color = R_PALETTE_VINIK24[(i / 3) % (ArrayCount(R_PALETTE_VINIK24) - 1)];
      R3D_RasterTriangleFlatCCW(frameBuffer, {a, b, c}, color);
      // R_RasterLine(frameBuffer, a.xy, b.xy, R_WHITE);
      // R_RasterLine(frameBuffer, b.xy, c.xy, R_WHITE);
      // R_RasterLine(frameBuffer, c.xy, a.xy, R_WHITE);
    }
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

  auto modelPath = Win32_PathFromExe(&arena, StringLit("../Suzanne/Suzanne.gltf"));
  auto modelMesh = LoadMesh(&arena, modelPath.ptr);

  ShowWindow(hWnd, SW_SHOWMAXIMIZED);

  auto timeNow = Win32_NowNanoseconds();
  auto timeLast = timeNow;

  R3D_Camera camera = {.position = {1, 1, 4}};
  f32 meshAngle = 0;

  for (; !g.windowState.destroyed;)
  {
    timeNow = Win32_NowNanoseconds();
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    Win32_PollMessages(&g.inputState);

    CameraUpdate(&camera, frameTime);
    meshAngle += frameTime;

    {
      auto hDC = GetDC(hWnd);
      Defer(ReleaseDC(hWnd, hDC););

      R_Clear(&frameBuffer, {0x44, 0x33, 0x22, 0xff}, true);
      Defer(R_PresentFrameBuffer(hWnd, hDC, frameBuffer););

      // auto viewport = R_PresentViewport(g.windowState.clientSize, frameBuffer.size);
      // auto mouseNDC = R_PositionToNDC(R_PointFrom2I32(g.inputState.mousePosition), viewport);
      // auto mouse2F32 = R_NDCToPosition(mouseNDC, {{}, frameBuffer.size});

      auto frameBufferSize2F32 = R_PointFrom2I32(frameBuffer.size);
      auto aspectRatio = frameBufferSize2F32.width / frameBufferSize2F32.height;
      auto projection = WorldPerspectiveMatrix(RadiansFromDegreesF32(60), aspectRatio);
      auto viewProjection = projection * R3D_CameraViewMatrix(&camera);
      auto model = Matrix4x4Translate({0, 1, 0}) *
                   Matrix4x4RotationY(meshAngle) *
                   // Matrix4x4Shear(0.5f, 0, 0, 0, 0, 0) *
                   Matrix4x4Scale({1, 1, 1});

      DrawMesh(&frameBuffer, viewProjection * model, modelMesh);
      DrawWorldAxes(&frameBuffer, viewProjection, R_FONT_3DFX);
    }

    // DwmFlush();
  }
}
