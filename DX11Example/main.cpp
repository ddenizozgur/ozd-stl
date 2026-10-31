#include "../Base/File.hpp"
#include "../Base/Math.hpp"
#include "../Base/Time.hpp"
#include "../DX11Stuff/Buffers.hpp"
#include "../DX11Stuff/Presets.hpp"
#include "../DX11Stuff/RenderTarget.hpp"
#include "../DX11Stuff/Shader.hpp"
#include "../DX11Stuff/Swapchain.hpp"
#include "../DX11Stuff/Texture.hpp"
#include "../Win32Stuff/Com.hpp"
#include "../Win32Stuff/Image.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WindowRelated.hpp"

struct Example_Globals
{
  bool shouldClose = false;
  Vector2<u32> clientSize = {};
};
inline Example_Globals g;

inline LRESULT CALLBACK ExampleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

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

struct alignas(16) Frame
{
  f32 aspect;
  f32 imageAspect;
  f32 time;
};

inline auto WIN32_NON_UNICODE_CURSOR_ARROW = MAKEINTRESOURCEA(32512);

int main()
{
  auto com = Win32_InitComOnThread(Com_Apartment_SingleThreaded);
  Defer(Win32_UninitComOnThread(&com););

  HINSTANCE hInstance = GetModuleHandleW(nullptr);
  WNDCLASSW wndClass = {
    .lpfnWndProc = ExampleWndProc,
    .hInstance = hInstance,
    .hCursor = LoadCursorA(nullptr, WIN32_NON_UNICODE_CURSOR_ARROW),
    .lpszClassName = L"DX11ExampleClass",
  };
  RegisterClassW(&wndClass);

  DWORD dwStyle = WS_OVERLAPPEDWINDOW;
  DWORD dwExStyle = WS_EX_NOREDIRECTIONBITMAP;
  RECT wndRect = {0, 0, 1280, 720};
  AdjustWindowRectEx(&wndRect, dwStyle, false, dwExStyle);
  auto hWnd = CreateWindowExW(dwExStyle, wndClass.lpszClassName, L"DX11Example", dwStyle,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                              nullptr, nullptr, hInstance, nullptr);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_Acrylic);

  auto factory6 = DX11_CreateFactory6();
  Defer(Win32_ComSafeRelease(&factory6););

  auto [gpu, gpuOk] = DX11_CreateGPU(factory6);
  auto [swapchain, swapchainOk] = DX11_CreateSwapchain(factory6, gpu, hWnd, DX11_Swapchain_Flag_Srgb);
  Defer(DX11_ReleaseGPU(&gpu);
        DX11_ReleaseSwapchain(&swapchain););

  auto arena = ArenaInit();
  String shaderPath = Win32_PathFromExe(&arena, StringLit("../Shaders/Textured.hlsl"));
  auto [vsBytes, vsOk] = DX11_CompileShaderFromFile(&arena, shaderPath, "VSMain", DX11_VERTEX_SHADER_TARGET);
  auto [psBytes, psOk] = DX11_CompileShaderFromFile(&arena, shaderPath, "PSMain", DX11_PIXEL_SHADER_TARGET);
  auto vertexShader = DX11_CreateVertexShader(gpu, vsBytes);
  auto pixelShader = DX11_CreatePixelShader(gpu, psBytes);
  Defer(Win32_ComSafeRelease(&vertexShader);
        Win32_ComSafeRelease(&pixelShader););

  String imagePath = Win32_PathFromExe(&arena, StringLit("../Assets/Pattern.png"));
  auto [fileBytes, fileOk] = Win32_ReadEntireFile(&arena, imagePath);
  auto [image, imageOk] = Win32_DecodeImageAs(&arena, fileBytes, Image_Format_RGBA8Premultiplied);
  DXGI_FORMAT textureFormat = DX11_TextureFormatFromImage(image.format, true);
  auto [texture, textureOk] = DX11_CreateTexture2D(gpu, image.size, textureFormat, image.pixels, true);
  Defer(DX11_ReleaseTexture2D(&texture););

  auto sampler = DX11_CreateSamplerState(gpu, DX11_SAMPLER_DESC_LINEAR_WRAP);
  auto blend = DX11_CreateBlendState(gpu, DX11_BLEND_DESC_PREMULTIPLIED_ALPHA);
  auto [constants, constantsOk] = DX11_CreateConstantBuffer(gpu, sizeof(Frame));
  Defer(Win32_ComSafeRelease(&sampler);
        Win32_ComSafeRelease(&blend);
        DX11_ReleaseConstantBuffer(&constants););

  Vector2<u32> textureSize = DX11_GetTexture2DSize(texture);
  f32 imageAspect = static_cast<f32>(textureSize.width) / textureSize.height;

  ShowWindow(hWnd, SW_SHOW);
  auto timeStart = Win32_NowNanoseconds();
  auto timeLast = timeStart;

  for (; !g.shouldClose;)
  {
    DX11_WaitForBetterLatency(swapchain);

    for (MSG message; PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE);)
    {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
    DX11_ResizeSwapchain(gpu, &swapchain, g.clientSize);

    auto timeNow = Win32_NowNanoseconds();
    auto frameTime = static_cast<f32>(SecondsFromNano(timeNow - timeLast));
    auto time = static_cast<f32>(SecondsFromNano(timeNow - timeStart));
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    Vector2<u32> swapchainSize = DX11_GetSwapchainSize(swapchain);
    {
      Frame frame = {
        .aspect = static_cast<f32>(swapchainSize.width) / swapchainSize.height,
        .imageAspect = imageAspect,
        .time = time,
      };
      DX11_UpdateConstantBuffer(gpu, constants, SliceAsBytes(Slice<Frame>{&frame, 1}));
    }

    {
      DX11_ClearBackbuffer(gpu, swapchain, {0.05f, 0.05f, 0.08f, 1});
      DX11_SetBackbufferAsRenderTarget(gpu, swapchain);

      auto context = gpu.immediateContext1;
      context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      context->VSSetShader(vertexShader, nullptr, 0);
      context->VSSetConstantBuffers(0, 1, &constants.buffer);
      context->PSSetShader(pixelShader, nullptr, 0);
      context->PSSetShaderResources(0, 1, &texture.view);
      context->PSSetSamplers(0, 1, &sampler);
      context->OMSetBlendState(blend, nullptr, 0xFFFFFFFF);
      context->Draw(6, 0);
    }

    DX11_PresentSwapchain(swapchain, 0);
  }

  DX11_ReportLiveObjects();
}

inline LRESULT CALLBACK ExampleWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  switch (message)
  {
  case WM_SIZE:
    g.clientSize = {LOWORD(lParam), HIWORD(lParam)};
    break;
  case WM_DESTROY:
    g.shouldClose = true;
    break;
  default:
    result = DefWindowProcW(hWnd, message, wParam, lParam);
    break;
  }
  return result;
}
