#define _CRT_SECURE_NO_WARNINGS

#include "../Base/File.hpp"
#include "../Base/Math.hpp"
#include "../Base/Random.hpp"
#include "../Base/Time.hpp"
#include "../DX11Stuff/RenderTarget.hpp"
#include "../DX11Stuff/Swapchain.hpp"
#include "../DX11Stuff/Texture.hpp"
#include "../Misc/Camera.hpp"
#include "../Win32Stuff/Com.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WicImage.hpp"
#include "../Win32Stuff/WindowRelated.hpp"
#include "DebugUI.hpp"
#include "R.hpp"
#include "R3D.hpp"

struct Example_Globals
{
  Win32_Window_State windowState;
  Win32_Input_State inputState;
  R3D_Frame_Constants frameConstants;
  f32 lightElevationDegrees = 55;
  f32 lightAzimuthDegrees = 140; // around Y, 0 is -Z
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

    auto title = StringFmt(scratch.arena, "DX11Example | %u FPS | %.4f ms",
                           frameCounter, frameTime * Thousand(1));
    Win32_SetWindowText(hWnd, title);

    elapsedTime -= 1;
    frameCounter = 0;
  }

  elapsedTime += frameTime;
  frameCounter += 1;
}

inline constexpr f32 CAMERA_MOVE_SPEED = 3;
inline constexpr f32 CAMERA_LOOK_SPEED = 0.003f;
inline constexpr f32 CAMERA_PITCH_LIMIT = RadiansFromDegreesF32(89);

inline void CameraUpdate(Camera3D *camera, const Win32_Input_State *input, f32 frameTime)
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

int main()
{
  // SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  auto com = Win32_InitComOnThread(Com_Apartment_SingleThreaded);
  Defer(Win32_UninitComOnThread(&com););

  auto [classId, classOk] = Win32_RegisterWindowClass(StringLit("DX11ExampleClass"), ExampleWndProc);
  auto hWnd = Win32_CreateWindow(classId, StringLit("DX11Example"), {1600, 900},
                                 Window_Kind_Normal, WS_EX_NOREDIRECTIONBITMAP);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic);
  Win32_SetWindowCorners(hWnd, Window_Corners_RoundSmall);

  auto instance = DX11_CreateInstance();
  Defer(DX11_ReleaseInstance(&instance););

  auto [gpu, gpuOk] = DX11_CreateGPU(instance);
  auto [swapchain, swapchainOk] = DX11_CreateSwapchain(instance, gpu, hWnd);
  Defer(DX11_ReleaseGPU(&gpu);
        DX11_ReleaseSwapchain(&swapchain););

  auto arena = ArenaInit();
  auto dwrite = Win32_CreateDWrite();
  // String fontPaths[] = {Win32_PathFromExe(&arena, StringLit("../Assets/JetBrainsMono-Regular.ttf"))};
  // auto fontFiles = Win32_DWriteCollectionFromFiles(dwrite, SliceFromArray(fontPaths));
  // auto [font, fontOk] = Win32_DWriteFontFromCollection(fontFiles, StringLit("JetBrains Mono"));
  auto fontFiles = Win32_DWriteCollectionFromSystem(dwrite);
  auto [font, fontOk] = Win32_DWriteFontFromCollection(fontFiles, StringLit("Consolas"));
  Defer(Win32_DWriteReleaseFont(&font);
        Win32_DWriteReleaseCollection(dwrite, &fontFiles);
        Win32_ReleaseDWrite(&dwrite););
  auto [r, rOk] = R_Init(&arena, gpu, dwrite);
  Defer(R_Release(&r););

  auto [r3d, r3dOk] = R3D_Init(gpu);
  // auto chairPath = Win32_PathFromExe(&arena, StringLit("../Assets/Chair/Chair.gltf"));
  auto chairPath = Win32_PathFromExe(&arena, StringLit("../Assets/DamagedHelmet/DamagedHelmet.gltf"));
  auto [chair, chairOk] = R3D_LoadMesh(gpu, chairPath);
  auto [depth, depthOk] = DX11_CreateDepthTarget(gpu, DX11_GetSwapchainSize(swapchain));
  Defer(R3D_Release(&r3d);
        R3D_ReleaseMesh(&chair);
        DX11_ReleaseDepthTarget(&depth););

  Camera3D camera = {.position = {0, 1, 3}};
  Debug_UI debug = {.panelPosition = {16, 16}};

  ShowWindow(hWnd, SW_SHOWMAXIMIZED);
  // ShowWindow(hWnd, SW_SHOW);
  auto timeStart = Win32_NowNanoseconds();
  auto timeLast = timeStart;

  for (; !g.windowState.destroyed;)
  {
    auto timeNow = Win32_NowNanoseconds();
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    DX11_WaitForBetterLatency(swapchain); // call it before handling input
    Win32_PollMessages(&g.inputState);

    DX11_ResizeSwapchain(gpu, &swapchain, Vector2Cast<u32>(g.windowState.clientSize));
    DX11_ClearBackbuffer(gpu, swapchain, {0.1f, 0.2f, 0.3f, 1});

    auto backbufferSize = DX11_GetSwapchainSize(swapchain);
    DX11_ResizeDepthTarget(gpu, &depth, backbufferSize);
    DX11_ClearDepthTarget(gpu, depth);

    auto backbufferSizeF32 = Vector2Cast<f32>(backbufferSize);
    {
      CameraUpdate(&camera, &g.inputState, frameTime);
      f32 aspect = backbufferSizeF32.width / backbufferSizeF32.height;
      auto projectionMatrix = Matrix4x4PerspectiveRH(RadiansFromDegreesF32(60), aspect, 0.1f, 100);
      auto cameraMatrix = Camera3DViewMatrix(&camera);
      g.frameConstants.viewProjectionMatrix = projectionMatrix * cameraMatrix;

      f32 elevation = RadiansFromDegreesF32(g.lightElevationDegrees);
      f32 azimuth = RadiansFromDegreesF32(g.lightAzimuthDegrees);
      g.frameConstants.directionToLight = DirectionFromYawPitch(azimuth, elevation);

      DX11_SetBackbufferAsRenderTarget(gpu, swapchain, depth);
      R3D_Bind(&r3d, g.frameConstants);

      R3D_DrawMesh(&r3d, chair, {.modelMatrix = Matrix4x4RotationX(RadiansFromDegreesF32(90))});
      R3D_DrawMesh(&r3d, chair, {.modelMatrix = Matrix4x4Translate({3, 0, 0}) * Matrix4x4RotationX(RadiansFromDegreesF32(90))});
    }

    {
      DX11_SetBackbufferAsRenderTarget(gpu, swapchain);
      R_Bind(&r, backbufferSizeF32);
      Defer(R_Flush(&r););

      {
        auto scratch = ScratchBegin();
        Defer(ScratchEnd(scratch););

        Debug_Begin(&debug, &g.inputState, &r, font, StringLit("Debug"));

        Debug_Checkbox(&debug, &g.frameConstants.showAlbedo, StringLit("Show albedo"));

        if (Debug_Button(&debug, StringFromDiffuseKind(g.frameConstants.diffuseKind)))
        {
          g.frameConstants.diffuseKind = (g.frameConstants.diffuseKind + 1) % Diffuse_Kind_COUNT;
        }

        Debug_SliderF32(&debug, &g.lightElevationDegrees, 0, 90, StringLit("Light elevation"));
        Debug_Label(&debug, StringFmt(scratch.arena, "%.2f", g.lightElevationDegrees));
        Debug_SliderF32(&debug, &g.lightAzimuthDegrees, 0, 360, StringLit("Light azimuth"));
        Debug_Label(&debug, StringFmt(scratch.arena, "%.2f", g.lightAzimuthDegrees));
        Debug_SliderF32(&debug, &g.frameConstants.lightIntensity, 0, 10, StringLit("Light intensity"));
        Debug_Label(&debug, StringFmt(scratch.arena, "%.2f", g.frameConstants.lightIntensity));
        Debug_SliderF32(&debug, &g.frameConstants.ambient, 0, 1, StringLit("Ambient"));
        Debug_Label(&debug, StringFmt(scratch.arena, "%.2f", g.frameConstants.ambient));

        auto debugHasMouse = Debug_End(&debug);
        (void)debugHasMouse;
      }
    }

    DX11_PresentSwapchain(swapchain, 0);
    DX11_FlushInfoQueue(instance);
  }
}