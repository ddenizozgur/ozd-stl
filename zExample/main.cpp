#define _CRT_SECURE_NO_WARNINGS

#include "../Base/File.hpp"
#include "../Base/Time.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WicImage.hpp"
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
  Vector2<f32> screen = R_NDCToPosition(clip.xy / clip.w, {.position = {}, .size = frameBuffer->size});
  f32 depth = clip.z / clip.w;
  return {{screen.x, screen.y, depth}, true};
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

  R_RasterLine(frameBuffer,
               R_NDCToPosition(clipA.xy / clipA.w, {.position = {}, .size = frameBuffer->size}),
               R_NDCToPosition(clipB.xy / clipB.w, {.position = {}, .size = frameBuffer->size}),
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
  for (usize i = 0; i < mesh.positions.count; i += 1)
  {
    projected[i] = ProjectToScreen(frameBuffer, mvp, mesh.positions[i]);
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
    }
  }
}

inline void RasterImage(R_FrameBuffer *frameBuffer, const Image *image,
                        Rect<i32> srcRect, Rect<i32> dstRect)
{
  assert(image->format == Image_Format_BGRA8 && "Image must be decoded as Image_Format_BGRA8");

  auto imageSize = Vector2Cast<i32>(image->size);
  if (srcRect.width <= 0 || srcRect.height <= 0)
  {
    srcRect.position = {};
    srcRect.size = imageSize;
  }

  auto bottomRight = dstRect.position + dstRect.size;
  i32 left = Maximum(0, dstRect.x);
  i32 right = Minimum(bottomRight.x, frameBuffer->size.width);
  i32 top = Maximum(0, dstRect.y);
  i32 bottom = Minimum(bottomRight.y, frameBuffer->size.height);
  if (left >= right)
  {
    return;
  }

  usize fbWidth = frameBuffer->size.width;
  i32 imagePitch = imageSize.width;

  auto srcPixels = reinterpret_cast<const R_B8G8R8A8 *>(image->pixels.ptr);
  srcPixels += srcRect.y * imagePitch + srcRect.x;

  f32 dstStartX = static_cast<f32>(dstRect.x);
  f32 dstEndX = static_cast<f32>(bottomRight.x);
  f32 dstStartY = static_cast<f32>(dstRect.y);
  f32 dstEndY = static_cast<f32>(bottomRight.y);
  auto dstPixels = frameBuffer->colorData.ptr;

  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto srcColumns = ArenaPushSlice<i32>(scratch.arena, right - left);
  for (i32 x = left; x < right; x += 1)
  {
    f32 u = InverseLerpF32(dstStartX, dstEndX, x + 0.5f);
    srcColumns[x - left] = FloorF32(u * srcRect.width);
  }

  for (i32 y = top; y < bottom; y += 1)
  {
    auto row = dstPixels + y * fbWidth + left;
    f32 v = InverseLerpF32(dstStartY, dstEndY, y + 0.5f);
    i32 srcY = FloorF32(v * srcRect.height);
    auto srcRow = srcPixels + srcY * imagePitch;

    for (usize i = 0; i < srcColumns.count; i += 1)
    {
      row[i] = srcRow[srcColumns[i]];
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

  auto modelPath = Win32_PathFromExe(&arena, StringLit("../Suzanne/suzanne.gltf"));
  auto modelMesh = LoadMesh(&arena, modelPath.ptr);

  /*
  auto imgPath = Win32_PathFromExe(&arena, StringLit("../Suzanne/Suzanne_BaseColor.png"));
  auto [fileBytes, fileOk] = Win32_ReadEntireFile(&arena, imgPath);
  auto [img, imgOk] = Win32_WicDecodeImageAs(&arena, fileBytes, Image_Format_BGRA8);
*/

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
      // auto mouseNDC = R_PositionToNDC(Vector2Cast<f32>(g.inputState.mousePosition), viewport);
      // auto mouse2F32 = R_NDCToPosition(mouseNDC, {{}, frameBuffer.size});

      auto fbSizeF32 = Vector2Cast<f32>(frameBuffer.size);
      auto aspectRatio = fbSizeF32.width / fbSizeF32.height;
      auto projection = WorldPerspectiveMatrix(RadiansFromDegreesF32(60), aspectRatio);
      auto viewProjection = projection * R3D_CameraViewMatrix(&camera);
      auto model = // Matrix4x4Translate({0, 1, 0}) *
        Matrix4x4RotationY(meshAngle) *
        // Matrix4x4RotationY(RadiansFromDegreesF32(180)) *
        // Matrix4x4Shear(0.5f, 0, 0, 0, 0, 0) *
        Matrix4x4Scale({1, 1, 1});

      DrawMesh(&frameBuffer, viewProjection * model, modelMesh);
      DrawWorldAxes(&frameBuffer, viewProjection, R_FONT_3DFX);

      // auto imgSize = Vector2Cast<i32>(img.size);
      // RasterImage(&frameBuffer, &img,
      //             {.position = {}, .size = imgSize / 4},
      //             {.position = {20, 20}, .size = imgSize / 4});
    }

    // DwmFlush();
  }
}

/*
[[nodiscard]]
inline Pair<Slice<u8>, bool>
GltfImageBytes(Arena_Allocator *arena, const cgltf_image *image, String gltfPath)
{
  if (image->buffer_view != nullptr)
  {
    auto view = image->buffer_view;
    auto bytes = const_cast<u8 *>(cgltf_buffer_view_data(view));
    return {{bytes, view->size}, true};
  }

  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  char *relative = image->uri;
  auto relativeCount = static_cast<i32>(cgltf_decode_uri(relative));
  String folder = StringBeforeLastInclusive(gltfPath, StringLit("/\\"));
  String path = StringFmt(scratch.arena, "%.*s%.*s", StringArgs(folder), relativeCount, relative);
  return Win32_ReadEntireFile(arena, path);
}

[[nodiscard]]
inline Pair<Image, bool>
LoadAlbedo(Arena_Allocator *arena, const cgltf_primitive *primitive, String gltfPath)
{
  auto material = primitive->material;
  if (material == nullptr)
  {
    return {};
  }

  auto texture = material->pbr_metallic_roughness.base_color_texture.texture;
  if (texture == nullptr && material->has_pbr_specular_glossiness)
  {
    texture = material->pbr_specular_glossiness.diffuse_texture.texture;
  }
  if (texture == nullptr || texture->image == nullptr)
  {
    return {};
  }

  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  auto [bytes, bytesOk] = GltfImageBytes(scratch.arena, texture->image, gltfPath);
  if (!bytesOk)
  {
    return {};
  }
  return Win32_WicDecodeImageAs(arena, bytes, Image_Format_BGRA8);
}

[[nodiscard]]
inline Slice<Vector2<f32>> LoadTexcoords(Arena_Allocator *arena, const cgltf_primitive *primitive, i32 set)
{
  auto accessor = cgltf_find_accessor(primitive, cgltf_attribute_type_texcoord, set);
  if (accessor == nullptr)
  {
    return {};
  }
  auto texcoords = ArenaPushSlice<Vector2<f32>>(arena, accessor->count);
  cgltf_accessor_unpack_floats(accessor, texcoords.ptr->data, texcoords.count * 2);
  return texcoords;
}
*/