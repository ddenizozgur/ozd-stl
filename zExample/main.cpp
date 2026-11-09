#define _CRT_SECURE_NO_WARNINGS

#include "../Base/File.hpp"
#include "../Base/Time.hpp"
#include "../Misc/Camera.hpp"
#include "../Misc/GLTF.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WicImage.hpp"
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

struct Mesh
{
  Slice<Vector3<f32>> positions;
  Slice<Vector2<f32>> texcoords;
  Slice<u32> indices;
  Pair<Image, bool> albedo;
};

inline Mesh LoadMesh(Arena_Allocator *arena, String path)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  auto pathCStr = CStringFrom(scratch.arena, path);

  cgltf_options options = {};
  cgltf_data *data = nullptr;
  if (cgltf_parse_file(&options, pathCStr, &data) != cgltf_result_success)
  {
    return {};
  }
  Defer(cgltf_free(data););

  if (cgltf_load_buffers(&options, data, pathCStr) != cgltf_result_success ||
      cgltf_validate(data) != cgltf_result_success)
  {
    return {};
  }

  auto primitive = &data->meshes[0].primitives[0];
  return {
    .positions = GLTF_LoadPositions(arena, primitive),
    .texcoords = GLTF_LoadTexcoords(arena, primitive, 0),
    .indices = GLTF_LoadIndices(arena, primitive),
    .albedo = GLTF_LoadAlbedo(arena, primitive, Image_Format_BGRA8, path),
  };
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

inline void Camera3DUpdate(Camera3D *camera, f32 frameTime)
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

[[nodiscard]]
inline Pair<Vector4<f32>, bool> ProjectToScreen(const R_FrameBuffer *frameBuffer,
                                                const Matrix4x4 &toClip, Vector3<f32> position)
{
  Vector4<f32> point = {.w = 1};
  point.xyz = position;

  auto clip = toClip * point;
  if (clip.w < WORLD_NEAR_Z)
  {
    return {};
  }
  f32 oneOverW = 1 / clip.w;
  Vector2<f32> screen = NDCToPosition(clip.xy * oneOverW, {.position = {}, .size = frameBuffer->size});
  f32 depth = clip.z * oneOverW;
  return {{screen.x, screen.y, depth, oneOverW}, true};
}

inline void DrawWorldRect(R_FrameBuffer *frameBuffer, const Matrix4x4 &viewProjection,
                          Vector3<f32> world, Vector2<i32> rectSize, R_B8G8R8A8 color)
{
  if (auto [screen, ok] = ProjectToScreen(frameBuffer, viewProjection, world); ok)
  {
    auto rect = Rect<i32>{.position = R_PointFrom2F32(screen.xy) - rectSize / 2, .size = rectSize};
    R_FillRect(frameBuffer, rect, color);
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

  auto nearFraction = InverseLerpF32(clipA.w, clipB.w, WORLD_NEAR_Z);
  auto clipOnNear = LerpF32(clipA, clipB, nearFraction);
  if (clipA.w < WORLD_NEAR_Z)
  {
    clipA = clipOnNear;
  }
  else if (clipB.w < WORLD_NEAR_Z)
  {
    clipB = clipOnNear;
  }

  auto viewport = Rect<i32>{.position = {}, .size = frameBuffer->size};
  R_RasterLine(frameBuffer,
               NDCToPosition(clipA.xy / clipA.w, viewport),
               NDCToPosition(clipB.xy / clipB.w, viewport),
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

using Mesh_Draw_Kind = u32;
enum : Mesh_Draw_Kind
{
  Mesh_Draw_Kind_Default,
  Mesh_Draw_Kind_PS1,
  Mesh_Draw_Kind_Debug,
};

inline void DrawMesh(R_FrameBuffer *frameBuffer, const Matrix4x4 &mvp,
                     Mesh mesh, Mesh_Draw_Kind drawKind = Mesh_Draw_Kind_Default)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto projected = ArenaPushSlice<Pair<Vector4<f32>, bool>>(scratch.arena, mesh.positions.count);
  for (usize i = 0; i < mesh.positions.count; i += 1)
  {
    projected[i] = ProjectToScreen(frameBuffer, mvp, mesh.positions[i]);
  }

  for (usize i = 0; i < mesh.indices.count; i += 3)
  {
    u32 i0 = mesh.indices[i + 0];
    u32 i1 = mesh.indices[i + 1];
    u32 i2 = mesh.indices[i + 2];

    auto [a, aOk] = projected[i0];
    auto [b, bOk] = projected[i1];
    auto [c, cOk] = projected[i2];

    if (aOk && bOk && cOk)
    {
      Vector4<f32> positions[3] = {a, b, c};
      switch (drawKind)
      {
      case Mesh_Draw_Kind_Default:
        if (mesh.albedo.second)
        {
          Vector2<f32> texcoords[3] = {mesh.texcoords[i0], mesh.texcoords[i1], mesh.texcoords[i2]};
          R3D_RasterTriangleCCW(frameBuffer, &mesh.albedo.first, {positions, texcoords});
          break;
        }
      case Mesh_Draw_Kind_PS1:
        if (mesh.albedo.second)
        {
          Vector2<f32> texcoords[3] = {mesh.texcoords[i0], mesh.texcoords[i1], mesh.texcoords[i2]};
          R3D_RasterTriangleCCW_PS1Style(frameBuffer, &mesh.albedo.first, {positions, texcoords});
          break;
        }
      case Mesh_Draw_Kind_Debug:
      {
        auto color = R_PALETTE_VINIK24[(i / 3) % (ArrayCount(R_PALETTE_VINIK24) - 1)];
        R3D_RasterTriangleFlatCCW(frameBuffer, {a.xyz, b.xyz, c.xyz}, color);
        break;
      }
      default:
        break;
      }
    }
  }
}

int main()
{
  auto com = Win32_InitComOnThread(Com_Apartment_SingleThreaded);
  Defer(Win32_UninitComOnThread(&com););

  Vector2<i32> windowSize = {1600, 900};
  auto [classId, classOk] = Win32_RegisterWindowClass(StringLit("ExampleClass"), ExampleWndProc);
  auto hWnd = Win32_CreateWindow(classId, StringLit("SomeApp"), windowSize, Window_Kind_Normal);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic, true);

  auto arena = ArenaInit();
  auto frameBuffer = R_CreateFrameBuffer(&arena, windowSize / 2);

  auto modelPath = Win32_PathFromExe(&arena, StringLit("../Suzanne/Suzanne.gltf"));
  auto modelMesh = LoadMesh(&arena, modelPath);

  ShowWindow(hWnd, SW_SHOWMAXIMIZED);

  auto timeNow = Win32_NowNanoseconds();
  auto timeLast = timeNow;

  Camera3D camera = {.position = {1, 1, 4}};
  f32 meshAngle = 0;

  for (; !g.windowState.destroyed;)
  {
    timeNow = Win32_NowNanoseconds();
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    Win32_PollMessages(&g.inputState);

    Camera3DUpdate(&camera, frameTime);
    meshAngle += frameTime;

    {
      auto hDC = GetDC(hWnd);
      Defer(ReleaseDC(hWnd, hDC););

      R_Clear(&frameBuffer, {0x44, 0x33, 0x22, 0xff}, true);
      Defer(R_PresentFrameBuffer(hWnd, hDC, frameBuffer););

      // R_HexDump(&frameBuffer, R_FONT_3DFX, {}, (void *)main, 8, 64);

      auto fbSizeF32 = Vector2Cast<f32>(frameBuffer.size);
      auto aspectRatio = fbSizeF32.width / fbSizeF32.height;
      auto projection = WorldPerspectiveMatrix(RadiansFromDegreesF32(60), aspectRatio);
      auto viewProjection = projection * Camera3DViewMatrix(&camera);
      auto model = // Matrix4x4Translate({0, 1, 0}) *
        Matrix4x4RotationY(meshAngle) *
        // Matrix4x4RotationY(RadiansFromDegreesF32(180)) *
        // Matrix4x4Shear(0.5f, 0, 0, 0, 0, 0) *
        Matrix4x4Scale({1, 1, 1});

      DrawMesh(&frameBuffer, viewProjection * model, modelMesh, Mesh_Draw_Kind_Default);
      DrawWorldAxes(&frameBuffer, viewProjection, R_FONT_3DFX);
    }

    // DwmFlush();
  }
}
