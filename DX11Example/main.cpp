#include "../Base/File.hpp"
#include "../Base/Math.hpp"
#include "../Base/Time.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WindowRelated.hpp"
#include "../DX11Stuff/Buffers.hpp"
#include "../DX11Stuff/RenderTarget.hpp"
#include "../DX11Stuff/Shader.hpp"
#include "../DX11Stuff/Swapchain.hpp"

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

struct Vertex
{
  f32 position[2];
  f32 color[4];
};

struct alignas(16) Frame
{
  f32 aspect;
  f32 time;
};

inline constexpr u32 INSTANCE_COUNT = 32;

inline auto WIN32_NON_UNICODE_CURSOR_ARROW = MAKEINTRESOURCEA(32512);

int main()
{
  HINSTANCE hInstance = GetModuleHandleW(nullptr);

  WNDCLASSW wndClass = {
      .lpfnWndProc = ExampleWndProc,
      .hInstance = hInstance,
      .hCursor = LoadCursorA(nullptr, WIN32_NON_UNICODE_CURSOR_ARROW),
      .lpszClassName = L"DX11ExampleClass",
  };
  RegisterClassW(&wndClass);

  DWORD dwStyle = WS_OVERLAPPEDWINDOW;
  DWORD dwExStyle = WS_EX_NOREDIRECTIONBITMAP; // flip model swapchains don't need GDI's copy of the window

  RECT wndRect = {0, 0, 1600, 900};
  AdjustWindowRectEx(&wndRect, dwStyle, false, dwExStyle);

  auto hWnd = CreateWindowExW(dwExStyle, wndClass.lpszClassName, L"DX11Example", dwStyle,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
                              nullptr, nullptr, hInstance, nullptr);
  Win32_SetDarkTheme(hWnd, Window_Backdrop_None);

  auto factory6 = DX11_CreateFactory6();
  Defer(Win32_ComSafeRelease(&factory6););

  auto [gpu, gpuOk] = DX11_CreateGPU(factory6);
  auto [swapchain, swapchainOk] = DX11_CreateSwapchain(factory6, gpu, hWnd);
  Defer(DX11_ReleaseGPU(&gpu);
        DX11_ReleaseSwapchain(&swapchain););

  auto arena = ArenaInit();
  String shaderPath = Win32_PathFromExe(&arena, StringLit("../Shaders/Triangle.hlsl"));
  auto [vsBytes, vsOk] = DX11_CompileShaderFromFile(&arena, shaderPath, "VSMain", DX11_VERTEX_SHADER_TARGET);
  auto [psBytes, psOk] = DX11_CompileShaderFromFile(&arena, shaderPath, "PSMain", DX11_PIXEL_SHADER_TARGET);
  auto vertexShader = DX11_CreateVertexShader(gpu, vsBytes);
  auto pixelShader = DX11_CreatePixelShader(gpu, psBytes);
  Defer(Win32_ComSafeRelease(&vertexShader);
        Win32_ComSafeRelease(&pixelShader););

  Vertex triangle[] = {
      {{0.0f, 1.0f}, {1, 0.2f, 0.2f, 1}},
      {{0.87f, -0.5f}, {0.2f, 1, 0.2f, 1}},
      {{-0.87f, -0.5f}, {0.2f, 0.4f, 1, 1}},
  };
  auto [vertices, verticesOk] = DX11_CreateStorageBuffer(gpu, SliceAsBytes(SliceFromArray(triangle)), sizeof(Vertex));
  auto [offsets, offsetsOk] = DX11_CreateTransientBuffer(gpu, INSTANCE_COUNT, sizeof(Vector2<f32>));
  auto [constants, constantsOk] = DX11_CreateConstantBuffer(gpu, sizeof(Frame));
  Defer(DX11_ReleaseStorageBuffer(&vertices);
        DX11_ReleaseTransientBuffer(&offsets);
        DX11_ReleaseConstantBuffer(&constants););

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
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    f32 time = SecondsFromNano(timeNow - timeStart);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    Vector2<u32> swapchainSize = DX11_GetSwapchainSize(swapchain);
    {
      Frame frame = {
          .aspect = static_cast<f32>(swapchainSize.width) / swapchainSize.height,
          .time = time,
      };
      DX11_UpdateConstantBuffer(gpu, constants, SliceAsBytes(Slice<Frame>{&frame, 1}));
    }

    {
      Vector2<f32> instanceOffsets[INSTANCE_COUNT];
      for (u32 i = 0; i < INSTANCE_COUNT; i += 1)
      {
        f32 angle = time * 0.5f + i * (6.2831853f / INSTANCE_COUNT);
        instanceOffsets[i] = {0.6f * cosf(angle), 0.6f * sinf(angle)};
      }
      DX11_UpdateTransientBuffer(gpu, offsets, SliceAsBytes(SliceFromArray(instanceOffsets)));
    }

    {
      DX11_ClearBackbuffer(gpu, swapchain, {0.1f, 0.2f, 0.3f, 1});
      DX11_SetBackbufferRenderPass(gpu, swapchain);

      auto context = gpu.immediateContext1;
      context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      context->VSSetShader(vertexShader, nullptr, 0);
      context->PSSetShader(pixelShader, nullptr, 0);
      ID3D11ShaderResourceView *views[] = {vertices.view, offsets.view};
      context->VSSetShaderResources(0, 2, views);
      context->VSSetConstantBuffers(0, 1, &constants.buffer);
      context->DrawInstanced(DX11_GetStorageBufferElementCount(vertices), INSTANCE_COUNT, 0, 0);
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
