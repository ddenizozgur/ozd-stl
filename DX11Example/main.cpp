#include "../Base/File.hpp"
#include "../Base/Math.hpp"
#include "../Base/Random.hpp"
#include "../Base/Time.hpp"
#include "../DX11Stuff/Buffers.hpp"
#include "../DX11Stuff/Presets.hpp"
#include "../DX11Stuff/RenderTarget.hpp"
#include "../DX11Stuff/Shader.hpp"
#include "../DX11Stuff/Swapchain.hpp"
#include "../DX11Stuff/Texture.hpp"
#include "../Win32Stuff/Com.hpp"
#include "../Win32Stuff/GlyphAtlas.hpp"
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WicImage.hpp"
#include "../Win32Stuff/WindowRelated.hpp"
#include "UI.hpp"

struct Example_Globals
{
  bool shouldClose = false;
  Vector2<u32> clientSize = {};
};
inline Example_Globals g;

inline constexpr Vector4<u8> PALETTE_VINIK24[] = {
  {0x6f, 0x67, 0x76, 0xff},
  {0x9a, 0x9a, 0x97, 0xff},
  {0xc5, 0xcc, 0xb8, 0xff},
  {0x8b, 0x55, 0x80, 0xff},
  {0xc3, 0x88, 0x90, 0xff},
  {0xa5, 0x93, 0xa5, 0xff},
  {0x66, 0x60, 0x92, 0xff},
  {0x9a, 0x4f, 0x50, 0xff},
  {0xc2, 0x8d, 0x75, 0xff},
  {0x7c, 0xa1, 0xc0, 0xff},
  {0x41, 0x6a, 0xa3, 0xff},
  {0x8d, 0x62, 0x68, 0xff},
  {0xbe, 0x95, 0x5c, 0xff},
  {0x68, 0xac, 0xa9, 0xff},
  {0x38, 0x70, 0x80, 0xff},
  {0x6e, 0x69, 0x62, 0xff},
  {0x93, 0xa1, 0x67, 0xff},
  {0x6e, 0xaa, 0x78, 0xff},
  {0x55, 0x70, 0x64, 0xff},
  {0x9d, 0x9f, 0x7f, 0xff},
  {0x7e, 0x9e, 0x99, 0xff},
  {0x5d, 0x68, 0x72, 0xff},
  {0x43, 0x34, 0x55, 0xff},
  {0x00, 0x00, 0x00, 0xff},
};

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

[[nodiscard]]
String WallLine(Arena_Allocator *arena, u64 lineIndex, usize maxChars)
{
  const char *GREETINGS[] = {"Merhaba", "Grüß Gott", "Bonjour", "¡Hola!", "Olá", "Cześć", "Dobrý den", "Szia",
                             "Bună ziua", "Hallå", "Tere", "Labas", "Sveiki", "Dobar dan", "Xin chào",
                             "Γειά σου", "Καλημέρα", "Привет", "Здравствуйте", "Добрий день", "Сайн уу",
                             "こんにちは", "ありがとう", "你好", "谢谢", "早上好"};

  auto random = RandomInit(lineIndex);
  auto line = FixedListInit(ArenaPushSlice<char>(arena, maxChars));
  for (;;)
  {
    auto wordIndex = RandomBelowU32(&random, static_cast<u32>(ArrayCount(GREETINGS)));
    String word = StringFromC(GREETINGS[wordIndex]);
    if (line.count + word.count + 1 > maxChars)
    {
      break;
    }
    FixedListAppend(&line, word);
    FixedListAppend(&line, StringLit(" "));
  }
  return SliceFromFixedList(line);
}

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

  auto instance = DX11_CreateInstance();
  Defer(DX11_ReleaseInstance(&instance););

  auto [gpu, gpuOk] = DX11_CreateGPU(instance);
  auto [swapchain, swapchainOk] = DX11_CreateSwapchain(instance, gpu, hWnd);
  Defer(DX11_ReleaseGPU(&gpu);
        DX11_ReleaseSwapchain(&swapchain););

  auto arena = ArenaInit();
  auto imagePath = Win32_PathFromExe(&arena, StringLit("../Assets/Pattern.png"));
  auto [fileBytes, fileOk] = Win32_ReadEntireFile(&arena, imagePath);
  auto [image, imageOk] = Win32_WicDecodeImageAs(&arena, fileBytes, Image_Format_RGBA8Premultiplied);
  auto textureFormat = DX11_TextureFormatFromImage(image.format, false);
  auto [texture, textureOk] = DX11_CreateTexture2D(gpu, image.pixels, image.size, textureFormat, true);
  Defer(DX11_ReleaseTexture2D(&texture););

  auto sampler = DX11_CreateSamplerState(gpu, DX11_SAMPLER_DESC_LINEAR_WRAP);
  auto blend = DX11_CreateBlendState(gpu, DX11_BLEND_DESC_PREMULTIPLIED_ALPHA);
  Defer(Win32_ComSafeRelease(&sampler);
        Win32_ComSafeRelease(&blend););

  auto textureSize = DX11_GetTexture2DSize(texture);
  auto imageAspect = static_cast<f32>(textureSize.width) / textureSize.height;

  auto uiShaderPath = Win32_PathFromExe(&arena, StringLit("../Shaders/UI.hlsl"));
  auto [uiVsBytes, uiVsOk] = DX11_CompileShaderFromFile(&arena, uiShaderPath, "VSMain", DX11_VERTEX_SHADER_TARGET);
  auto [uiPsBytes, uiPsOk] = DX11_CompileShaderFromFile(&arena, uiShaderPath, "PSMain", DX11_PIXEL_SHADER_TARGET);
  auto uiVertexShader = DX11_CreateVertexShader(gpu, uiVsBytes);
  auto uiPixelShader = DX11_CreatePixelShader(gpu, uiPsBytes);
  auto [uiConstants, uiConstantsOk] = DX11_CreateConstantBuffer(gpu, sizeof(UI_Constants));

  auto instances = ArenaPushSlice<UI_Instance>(&arena, 16 * 1024);
  auto [uiBatch, uiBatchOk] = UI_InitBatch(gpu, instances);
  Defer(Win32_ComSafeRelease(&uiVertexShader);
        Win32_ComSafeRelease(&uiPixelShader);
        DX11_ReleaseConstantBuffer(&uiConstants);
        UI_ReleaseBatch(&uiBatch););

  auto dwrite = Win32_CreateDWrite();
  auto blendParams = Win32_DWriteBlendParams(dwrite);
  auto systemFonts = Win32_DWriteCollectionFromSystem(dwrite);
  auto [font, fontOk] = Win32_DWriteFontFromCollection(systemFonts, StringLit("Microsoft YaHei"));
  auto [atlas, atlasOk] = GlyphAtlasInit(&arena, {1024, 1024});
  auto [atlasTexture, atlasTextureOk] = DX11_CreateTexture2D(gpu, atlas.coverage, atlas.size, DXGI_FORMAT_R8_UNORM, false, true);
  Defer(DX11_ReleaseTexture2D(&atlasTexture);
        GlyphAtlasRelease(&atlas);
        Win32_DWriteReleaseFont(&font);
        Win32_DWriteReleaseCollection(dwrite, &systemFonts);
        Win32_ReleaseDWrite(&dwrite););
  f32 pixelSize = 16;
  auto fontMetrics = Win32_DWriteFontMetrics(font, pixelSize);
  f32 charAdvance = 0;
  {
    auto scratch = ScratchBegin();
    Defer(ScratchEnd(scratch););

    auto shapedGlyph = Win32_DWriteShapeText(scratch.arena, dwrite, font, pixelSize, StringLit("B"));
    charAdvance = RoundHalfDownF32(shapedGlyph[0].advance);
  }

  ShowWindow(hWnd, SW_SHOWMAXIMIZED);
  auto timeStart = Win32_NowNanoseconds();
  auto timeLast = timeStart;

  for (; !g.shouldClose;)
  {
    auto timeNow = Win32_NowNanoseconds();
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    f32 time = SecondsFromNano(timeNow - timeStart);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    DX11_WaitForBetterLatency(swapchain); // call it before handling input
    for (MSG message; PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE);)
    {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
    DX11_ResizeSwapchain(gpu, &swapchain, g.clientSize);

    auto swapchainSize = DX11_GetSwapchainSize(swapchain);
    {
      UI_Constants constantsUI = {
        .gammaRatios = blendParams.gammaRatios,
        .screenSize = {static_cast<f32>(swapchainSize.width), static_cast<f32>(swapchainSize.height)},
        .enhancedContrast = blendParams.enhancedContrast,
      };
      DX11_UpdateConstantBuffer(gpu, uiConstants, SliceAsBytes(SliceMake(&constantsUI, 1)));

      DX11_ClearBackbuffer(gpu, swapchain, {0.1f, 0.2f, 0.3f, 1});
      DX11_SetBackbufferAsRenderTarget(gpu, swapchain);

      auto context = gpu.immediateContext1;
      context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      context->VSSetShader(uiVertexShader, nullptr, 0);
      context->VSSetConstantBuffers(0, 1, &uiConstants.buffer);
      context->VSSetShaderResources(0, 1, &uiBatch.gpuInstances.view);
      context->PSSetShader(uiPixelShader, nullptr, 0);
      context->PSSetConstantBuffers(0, 1, &uiConstants.buffer);
      ID3D11ShaderResourceView *pixelResourcesArray[] = {atlasTexture.view, texture.view};
      context->PSSetShaderResources(1, static_cast<u32>(ArrayCount(pixelResourcesArray)), pixelResourcesArray);
      context->PSSetSamplers(0, 1, &sampler);
      context->OMSetBlendState(blend, nullptr, 0xFFFFFFFF);

      auto scratch = ScratchBegin();
      Defer(ScratchEnd(scratch););

      Vector2<f32> screenSize = constantsUI.screenSize;

      f32 margin = 16;
      auto charsPerLine = static_cast<usize>((screenSize.width - margin * 2) / charAdvance);

      f32 scroll = time * 30;
      auto firstLine = static_cast<usize>(scroll / fontMetrics.lineHeightRounded);
      f32 lineOffset = scroll - firstLine * fontMetrics.lineHeightRounded;
      auto visibleLines = static_cast<usize>(screenSize.height / fontMetrics.lineHeightRounded) + 2;
      for (usize i = 0; i < visibleLines; i += 1)
      {
        String line = WallLine(scratch.arena, firstLine + i, charsPerLine);
        Vector4<u8> color = PALETTE_VINIK24[(firstLine + i) % ArrayCount(PALETTE_VINIK24)];
        Vector2<f32> pen = {margin, fontMetrics.ascent + i * fontMetrics.lineHeightRounded - lineOffset};
        UI_PushText(gpu, &uiBatch, &atlas, atlasTexture, dwrite, pen, font, pixelSize, color, line);
      }

      f32 imageHeight = screenSize.height * 0.6f;
      Vector2<f32> imageSize = {imageHeight * imageAspect, imageHeight};
      UI_PushImage(gpu, &uiBatch, (screenSize - imageSize) / 2.0f, imageSize);

      UI_Flush(gpu, &uiBatch);
    }

    DX11_PresentSwapchain(swapchain, 0);
    DX11_FlushInfoQueue(instance);
  }
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