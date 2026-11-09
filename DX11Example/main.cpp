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
#include "../Win32Stuff/Theme.hpp"
#include "../Win32Stuff/WicImage.hpp"
#include "../Win32Stuff/WindowRelated.hpp"
#include "DebugUI.hpp"
#include "R.hpp"

struct Example_Globals
{
  Win32_Window_State windowState;
  Win32_Input_State inputState;
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
  auto imagePath = Win32_PathFromExe(&arena, StringLit("../Assets/Pattern.png"));
  auto [fileBytes, fileOk] = Win32_ReadEntireFile(&arena, imagePath);
  auto [image, imageOk] = Win32_WicDecodeImageAs(&arena, fileBytes, Image_Format_RGBA8Premultiplied);
  auto textureFormat = DX11_TextureFormatFromImage(image.format, false);
  auto [texture, textureOk] = DX11_CreateTexture2D(gpu, image.pixels, image.size, textureFormat, true);
  Defer(DX11_ReleaseTexture2D(&texture););

  auto samplerLinearWrap = DX11_CreateSamplerState(gpu, DX11_SAMPLER_DESC_LINEAR_WRAP);
  auto blendPremultiplied = DX11_CreateBlendState(gpu, DX11_BLEND_DESC_PREMULTIPLIED_ALPHA);
  Defer(Win32_ComSafeRelease(&samplerLinearWrap);
        Win32_ComSafeRelease(&blendPremultiplied););

  auto textureSize = DX11_GetTexture2DSize(texture);
  auto imageAspect = static_cast<f32>(textureSize.width) / textureSize.height;

  auto shaderPath = Win32_PathFromExe(&arena, StringLit("../Shaders/R.hlsl"));
  auto [vsBytes, vsOk] = DX11_CompileShaderFromFile(&arena, shaderPath, "VSMain", DX11_VERTEX_SHADER_TARGET);
  auto [psBytes, psOk] = DX11_CompileShaderFromFile(&arena, shaderPath, "PSMain", DX11_PIXEL_SHADER_TARGET);
  auto vertexShader = DX11_CreateVertexShader(gpu, vsBytes);
  auto pixelShader = DX11_CreatePixelShader(gpu, psBytes);
  auto [constantsGpu, constantsGpuOk] = DX11_CreateConstantBuffer(gpu, sizeof(R_Constants));

  auto instances = ArenaPushSlice<R_Instance>(&arena, Kilobytes(32) / sizeof(R_Instance));
  auto [batch, batchOk] = R_InitBatch(gpu, instances);
  Defer(Win32_ComSafeRelease(&vertexShader);
        Win32_ComSafeRelease(&pixelShader);
        DX11_ReleaseConstantBuffer(&constantsGpu);
        R_ReleaseBatch(&batch););

  auto dwrite = Win32_CreateDWrite();
  auto blendParams = Win32_DWriteBlendParams(dwrite);
  // String fontPaths[] = {Win32_PathFromExe(&arena, StringLit("../Assets/JetBrainsMono-Regular.ttf"))};
  // auto fontFiles = Win32_DWriteCollectionFromFiles(dwrite, SliceFromArray(fontPaths));
  // auto [font, fontOk] = Win32_DWriteFontFromCollection(fontFiles, StringLit("JetBrains Mono"));
  auto fontFiles = Win32_DWriteCollectionFromSystem(dwrite);
  auto [font, fontOk] = Win32_DWriteFontFromCollection(fontFiles, StringLit("Consolas"));
  auto [atlas, atlasOk] = GlyphAtlasInit(&arena, {1024, 1024});
  auto [atlasTexture, atlasTextureOk] = DX11_CreateTexture2D(gpu, atlas.coverage, atlas.size, DXGI_FORMAT_R8_UNORM, false, true);
  Defer(DX11_ReleaseTexture2D(&atlasTexture);
        GlyphAtlasRelease(&atlas);
        Win32_DWriteReleaseFont(&font);
        Win32_DWriteReleaseCollection(dwrite, &fontFiles);
        Win32_ReleaseDWrite(&dwrite););

  bool showImage = true;
  Debug_UI debug = {.panelPosition = {16, 16}};

  ShowWindow(hWnd, SW_SHOWMAXIMIZED);
  // ShowWindow(hWnd, SW_SHOW);
  auto timeLast = Win32_NowNanoseconds();

  for (; !g.windowState.destroyed;)
  {
    auto timeNow = Win32_NowNanoseconds();
    f32 frameTime = SecondsFromNano(timeNow - timeLast);
    timeLast = timeNow;
    UpdateTitle(hWnd, frameTime);

    DX11_WaitForBetterLatency(swapchain); // call it before handling input
    Win32_PollMessages(&g.inputState);
    auto swapchainSize = Vector2Cast<u32>(g.windowState.clientSize);
    DX11_ResizeSwapchain(gpu, &swapchain, swapchainSize);

    {
      R_Constants constantsFrame = {
        .gammaRatios = blendParams.gammaRatios,
        .screenSize = Vector2Cast<f32>(swapchainSize),
        .enhancedContrast = blendParams.enhancedContrast,
      };
      DX11_UpdateConstantBuffer(gpu, constantsGpu, SliceAsBytes(SliceMake(&constantsFrame, 1)));

      DX11_ClearBackbuffer(gpu, swapchain, {0.1f, 0.2f, 0.3f, 1});
      DX11_SetBackbufferAsRenderTarget(gpu, swapchain);

      auto context = gpu.immediateContext1;
      context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      context->VSSetShader(vertexShader, nullptr, 0);
      context->VSSetConstantBuffers(0, 1, &constantsGpu.buffer);
      context->VSSetShaderResources(0, 1, &batch.gpuInstances.view);
      context->PSSetShader(pixelShader, nullptr, 0);
      context->PSSetConstantBuffers(0, 1, &constantsGpu.buffer);
      context->PSSetShaderResources(1, 1, &atlasTexture.view);
      context->PSSetSamplers(0, 1, &samplerLinearWrap);
      context->OMSetBlendState(blendPremultiplied, nullptr, 0xFFFFFFFF);

      Vector2<f32> screenSize = constantsFrame.screenSize;

      if (showImage)
      {
        f32 imageHeight = screenSize.height * 0.6f;
        Vector2<f32> imageSize = {imageHeight * imageAspect, imageHeight};
        R_PushImage(gpu, &batch, texture, (screenSize - imageSize) / 2.0f, imageSize,
                    {0, 0}, Vector2Cast<f32>(textureSize), {255, 255, 255, 255}, 24);
      }

      {
        Debug_Renderer debugRenderer = {
          .gpu = gpu,
          .batch = &batch,
          .atlas = &atlas,
          .atlasTexture = atlasTexture,
          .dwrite = dwrite,
          .font = font,
        };
        Debug_Begin(&debug, &g.inputState, debugRenderer, StringLit("Debug"));

        Debug_Checkbox(&debug, &showImage, StringLit("Show image"));
        auto debugHasMouse = Debug_End(&debug);
        (void)debugHasMouse;
      }

      R_Flush(gpu, &batch);
    }

    DX11_PresentSwapchain(swapchain, 0);
    DX11_FlushInfoQueue(instance);
  }
}