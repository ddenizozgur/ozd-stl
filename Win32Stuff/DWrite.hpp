#pragma once

// requires Windows 10 version 1703.

#include "../Base/Math.hpp"
#include "../Base/Unicode.hpp"
#include "Com.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwrite_3.h>
#pragma comment(lib, "dwrite")

struct Win32_DWrite
{
  IDWriteFactory5 *factory5;
  IDWriteTextAnalyzer *analyzer;
};

struct Win32_DWrite_Collection
{
  IDWriteFontCollection1 *fonts;
  IDWriteInMemoryFontFileLoader *memoryLoader; // nullptr except from CollectionFromMemory
};

struct Win32_DWrite_Font
{
  IDWriteFontFace2 *face2;
};

// in pixels
struct Font_Metrics
{
  f32 ascent;
  f32 descent;
  f32 lineGap;
  f32 lineHeight; // ascent + descent + lineGap
};

struct Shaped_Glyph
{
  u16 index;
  f32 advance;
  Vector2<f32> offset;
};

struct Glyph_Grayscale
{
  Slice<u8> coverage; // rows top to bottom with no padding
  Vector2<u32> size;
  Vector2<i32> offset; // top-left of coverage
};

[[nodiscard]]
inline Win32_DWrite Win32_CreateDWrite()
{
  Win32_DWrite dwrite = {};
  DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                      __uuidof(IDWriteFactory5),
                      reinterpret_cast<IUnknown **>(&dwrite.factory5));
  dwrite.factory5->CreateTextAnalyzer(&dwrite.analyzer);
  return dwrite;
}

inline void Win32_ReleaseDWrite(Win32_DWrite *dwrite)
{
  Win32_ComSafeRelease(&dwrite->analyzer);
  Win32_ComSafeRelease(&dwrite->factory5);
}

[[nodiscard]]
inline Win32_DWrite_Collection Win32_DWriteCollectionFromSystem(Win32_DWrite dwrite)
{
  Win32_DWrite_Collection collection = {};
  dwrite.factory5->GetSystemFontCollection(false, &collection.fonts);
  return collection;
}

[[nodiscard]]
inline IDWriteFontCollection1 *
Win32_DWriteBuildCollection(Win32_DWrite dwrite, IDWriteFontSetBuilder1 *builder)
{
  IDWriteFontSet *set = nullptr;
  builder->CreateFontSet(&set);
  Defer(set->Release(););

  IDWriteFontCollection1 *fonts = nullptr;
  dwrite.factory5->CreateFontCollectionFromFontSet(set, &fonts);
  return fonts;
}

// .ttf, .otf or .ttc files. files that failed to load are skipped.
[[nodiscard]]
inline Win32_DWrite_Collection
Win32_DWriteCollectionFromFiles(Win32_DWrite dwrite, Slice<String> paths)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  IDWriteFontSetBuilder1 *builder = nullptr;
  dwrite.factory5->CreateFontSetBuilder(&builder);
  Defer(builder->Release(););

  for (auto path : paths)
  {
    auto path16 = Win32_String16From(scratch.arena, path);
    IDWriteFontFile *file = nullptr;
    if (SUCCEEDED(dwrite.factory5->CreateFontFileReference(path16.ptr, nullptr, &file)))
    {
      builder->AddFontFile(file);
      file->Release();
    }
  }
  return {Win32_DWriteBuildCollection(dwrite, builder)};
}

[[nodiscard]]
inline Win32_DWrite_Collection
Win32_DWriteCollectionFromMemory(Win32_DWrite dwrite, Slice<Slice<u8>> fonts)
{
  Win32_DWrite_Collection collection = {};
  dwrite.factory5->CreateInMemoryFontFileLoader(&collection.memoryLoader);
  dwrite.factory5->RegisterFontFileLoader(collection.memoryLoader);

  IDWriteFontSetBuilder1 *builder = nullptr;
  dwrite.factory5->CreateFontSetBuilder(&builder);
  Defer(builder->Release(););

  for (auto font : fonts)
  {
    auto size = static_cast<u32>(font.count);
    IDWriteFontFile *file = nullptr;
    collection.memoryLoader->CreateInMemoryFontFileReference(dwrite.factory5, font.ptr, size, nullptr, &file);
    builder->AddFontFile(file);
    file->Release();
  }
  collection.fonts = Win32_DWriteBuildCollection(dwrite, builder);
  return collection;
}

// release the fonts first
inline void Win32_DWriteReleaseCollection(Win32_DWrite dwrite, Win32_DWrite_Collection *collection)
{
  Win32_ComSafeRelease(&collection->fonts);
  if (collection->memoryLoader != nullptr)
  {
    dwrite.factory5->UnregisterFontFileLoader(collection->memoryLoader);
    Win32_ComSafeRelease(&collection->memoryLoader);
  }
}

// useful when the family names can't known upfront
[[nodiscard]]
inline Slice<String>
Win32_DWriteCollectionFamilyNames(Arena_Allocator *arena, Win32_DWrite_Collection collection)
{
  u32 familyCount = collection.fonts->GetFontFamilyCount();
  auto names = ArenaPushSlice<String>(arena, familyCount);
  if (names.ptr == nullptr)
  {
    return {};
  }

  for (u32 i = 0; i < familyCount; i += 1)
  {
    IDWriteFontFamily *family = nullptr;
    collection.fonts->GetFontFamily(i, &family);
    Defer(family->Release(););

    IDWriteLocalizedStrings *localizedNames = nullptr;
    family->GetFamilyNames(&localizedNames);
    Defer(localizedNames->Release(););

    u32 index = 0;
    BOOL exists = false;
    localizedNames->FindLocaleName(L"en-us", &index, &exists);
    if (!exists)
    {
      index = 0;
    }

    auto scratch = ScratchBegin();
    Defer(ScratchEnd(scratch, IsScratch(arena)););

    u32 length = 0;
    localizedNames->GetStringLength(index, &length);
    auto name16 = ArenaPushSlice<wchar_t>(scratch.arena, length + 1);
    localizedNames->GetString(index, name16.ptr, length + 1);
    names[i] = Win32_StringFrom16(arena, {name16.ptr, length});
  }
  return names;
}

// weight is 100 to 900, 400 is regular and 700 is bold.
// picks the closest weight and style, false when the collection has no family with that name.
[[nodiscard]]
inline Pair<Win32_DWrite_Font, bool>
Win32_DWriteFontFromCollection(Win32_DWrite_Collection collection,
                               String familyName, u32 weight = 400, bool italic = false)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto familyName16 = Win32_String16From(scratch.arena, familyName);
  u32 familyIndex = 0;
  BOOL exists = false;
  collection.fonts->FindFamilyName(familyName16.ptr, &familyIndex, &exists);
  if (!exists)
  {
    return {};
  }

  IDWriteFontFamily *family = nullptr;
  collection.fonts->GetFontFamily(familyIndex, &family);
  Defer(family->Release(););

  auto fontWeight = static_cast<DWRITE_FONT_WEIGHT>(weight);
  DWRITE_FONT_STYLE style = italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
  IDWriteFont *font = nullptr;
  family->GetFirstMatchingFont(fontWeight, DWRITE_FONT_STRETCH_NORMAL, style, &font);
  Defer(font->Release(););

  IDWriteFontFace *face = nullptr;
  font->CreateFontFace(&face);
  if (face == nullptr)
  {
    return {};
  }
  Defer(face->Release(););

  IDWriteFontFace2 *face2 = nullptr;
  face->QueryInterface(&face2);
  return {{face2}, true};
}

inline void Win32_DWriteReleaseFont(Win32_DWrite_Font *font)
{
  Win32_ComSafeRelease(&font->face2);
}

// pixelSize is the em size in pixels, the point size * dpi / 72
[[nodiscard]]
inline Font_Metrics Win32_DWriteFontMetrics(Win32_DWrite_Font font, f32 pixelSize)
{
  DWRITE_FONT_METRICS metrics = {};
  font.face2->GetMetrics(&metrics);
  f32 scale = pixelSize / metrics.designUnitsPerEm;
  f32 ascent = metrics.ascent * scale;
  f32 descent = metrics.descent * scale;
  f32 lineGap = metrics.lineGap * scale;
  return {ascent, descent, lineGap, ascent + descent + lineGap};
}

// 0 is the font's missing glyph
[[nodiscard]]
inline u16 Win32_DWriteGlyphIndex(Win32_DWrite_Font font, Rune rune)
{
  u16 index = 0;
  font.face2->GetGlyphIndices(&rune, 1, &index);
  return index;
}

// NOTE: scripts like Arabic and Devanagari not supported *yet*.
[[nodiscard]]
inline Slice<Shaped_Glyph>
Win32_DWriteShapeText(Arena_Allocator *arena, Win32_DWrite dwrite, Win32_DWrite_Font font,
                      f32 pixelSize, String text, bool codeLigatures = false)
{
  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  auto text16 = Win32_String16From(scratch.arena, text);
  auto textLength = static_cast<u32>(text16.count);
  if (textLength == 0)
  {
    return {};
  }

  DWRITE_SCRIPT_ANALYSIS script = {};
  // passing a feature list turns off every default feature if it doesn't in the list xd
  DWRITE_FONT_FEATURE noCodeLigatures[] = {
    {DWRITE_FONT_FEATURE_TAG_KERNING, 1},
    {DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES, 1},
    {DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES, 1},
    {DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, 0},
  };
  auto noCodeLigaturesCount = static_cast<u32>(ArrayCount(noCodeLigatures));
  DWRITE_TYPOGRAPHIC_FEATURES features = {noCodeLigatures, noCodeLigaturesCount};
  const DWRITE_TYPOGRAPHIC_FEATURES *featureRanges = &features;
  u32 featureRangeCount = !codeLigatures ? 1 : 0;

  auto clusters = ArenaPushSlice<u16>(scratch.arena, textLength);
  auto textProps = ArenaPushSlice<DWRITE_SHAPING_TEXT_PROPERTIES>(scratch.arena, textLength);
  Slice<u16> glyphs = {};
  Slice<DWRITE_SHAPING_GLYPH_PROPERTIES> glyphProps = {};
  u32 glyphCount = 0;
  // a rune can become several glyphs
  for (u32 maxGlyphCount = textLength * 3 / 2 + 16;; maxGlyphCount *= 2)
  {
    glyphs = ArenaPushSlice<u16>(scratch.arena, maxGlyphCount);
    glyphProps = ArenaPushSlice<DWRITE_SHAPING_GLYPH_PROPERTIES>(scratch.arena, maxGlyphCount);
    HRESULT result = dwrite.analyzer->GetGlyphs(text16.ptr, textLength, font.face2, false, false, &script, nullptr,
                                                nullptr, &featureRanges, &textLength, featureRangeCount, maxGlyphCount,
                                                clusters.ptr, textProps.ptr, glyphs.ptr, glyphProps.ptr, &glyphCount);
    if (result != E_NOT_SUFFICIENT_BUFFER)
    {
      break;
    }
  }

  auto advances = ArenaPushSlice<f32>(scratch.arena, glyphCount);
  auto offsets = ArenaPushSlice<DWRITE_GLYPH_OFFSET>(scratch.arena, glyphCount);
  dwrite.analyzer->GetGlyphPlacements(text16.ptr, clusters.ptr, textProps.ptr, textLength, glyphs.ptr, glyphProps.ptr,
                                      glyphCount, font.face2, pixelSize, false, false, &script, nullptr,
                                      &featureRanges, &textLength, featureRangeCount, advances.ptr, offsets.ptr);

  auto shaped = ArenaPushSlice<Shaped_Glyph>(arena, glyphCount);
  if (shaped.ptr == nullptr)
  {
    return {};
  }
  for (u32 i = 0; i < glyphCount; i += 1)
  {
    shaped[i] = {glyphs[i], advances[i], {offsets[i].advanceOffset, -offsets[i].ascenderOffset}};
  }
  return shaped;
}

// grayscale antialiased, the coverage has no gamma or contrast applied.
[[nodiscard]]
inline Pair<Glyph_Grayscale, bool>
Win32_DWriteRasterizeGlyph(Arena_Allocator *arena, Win32_DWrite dwrite,
                           Win32_DWrite_Font font, f32 pixelSize, u16 glyphIndex)
{
  Glyph_Grayscale glyph = {};

  DWRITE_RENDERING_MODE renderingMode = {};
  DWRITE_GRID_FIT_MODE gridFitMode = {};
  font.face2->GetRecommendedRenderingMode(pixelSize, 96, 96, nullptr, false, DWRITE_OUTLINE_THRESHOLD_ANTIALIASED,
                                          DWRITE_MEASURING_MODE_NATURAL, nullptr, &renderingMode, &gridFitMode);
  // DirectWrite draws glyphs as geometry in outline mode
  if (renderingMode == DWRITE_RENDERING_MODE_OUTLINE)
  {
    renderingMode = DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC;
  }

  DWRITE_GLYPH_RUN run = {
    .fontFace = font.face2,
    .fontEmSize = pixelSize,
    .glyphCount = 1,
    .glyphIndices = &glyphIndex,
  };
  IDWriteGlyphRunAnalysis *analysis = nullptr;
  dwrite.factory5->CreateGlyphRunAnalysis(&run, nullptr, renderingMode,
                                          DWRITE_MEASURING_MODE_NATURAL, gridFitMode,
                                          DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE, 0, 0, &analysis);
  Defer(analysis->Release(););

  RECT bounds = {};
  analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds);
  u32 width = bounds.right - bounds.left;
  u32 height = bounds.bottom - bounds.top;
  glyph.offset = {bounds.left, bounds.top};
  if (width == 0 || height == 0)
  {
    return {glyph, true};
  }

  auto pixels = ArenaPushSlice<u8>(arena, static_cast<usize>(width) * height);
  if (pixels.ptr == nullptr)
  {
    return {};
  }
  analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1, &bounds, pixels.ptr, static_cast<u32>(pixels.count));
  glyph.coverage = pixels;
  glyph.size = {width, height};
  return {glyph, true};
}

// apply to raw coverage when blending grayscale text
struct Win32_DWrite_Blend_Params
{
  Vector4<f32> gammaRatios;
  f32 enhancedContrast;
};

struct Win32_DWrite_Blend_Table_For_Cpu
{
  f32 alphas[256];
};

// Copyright (c) Microsoft Corporation, MIT license:
// https://github.com/microsoft/terminal/blob/main/src/renderer/atlas/dwrite_helpers.cpp
[[nodiscard]]
inline Win32_DWrite_Blend_Params Win32_DWriteBlendParams(Win32_DWrite dwrite)
{
  IDWriteRenderingParams *params = nullptr;
  dwrite.factory5->CreateRenderingParams(&params);
  Defer(params->Release(););

  IDWriteRenderingParams1 *params1 = nullptr;
  params->QueryInterface(&params1);
  Defer(params1->Release(););

  // rows are gamma 1.0 to 2.2 in 0.1 steps
  static constexpr Vector4<f32> GAMMA_RATIOS[13] = {
    {0.0000f, 0.0000f, 0.0000f, 0.0000f},
    {0.0166f, -0.0807f, 0.2227f, -0.0751f},
    {0.0350f, -0.1760f, 0.4325f, -0.1370f},
    {0.0543f, -0.2821f, 0.6302f, -0.1876f},
    {0.0739f, -0.3963f, 0.8167f, -0.2287f},
    {0.0933f, -0.5161f, 0.9926f, -0.2616f},
    {0.1121f, -0.6395f, 1.1588f, -0.2877f},
    {0.1300f, -0.7649f, 1.3159f, -0.3080f},
    {0.1469f, -0.8911f, 1.4644f, -0.3234f},
    {0.1627f, -1.0170f, 1.6051f, -0.3347f},
    {0.1773f, -1.1420f, 1.7385f, -0.3426f},
    {0.1908f, -1.2652f, 1.8650f, -0.3476f},
    {0.2031f, -1.3864f, 1.9851f, -0.3501f},
  };
  auto row = static_cast<i32>(RoundHalfUpF32(params1->GetGamma() * 10)) - 10;
  auto &ratios = GAMMA_RATIOS[Clamp(row, 0, 12)];

  // the table is in 8-bit fixed point
  f32 norm13 = 65536.0f / (255.0f * 255.0f);
  f32 norm24 = 256.0f / 255.0f;
  Vector4<f32> scale = {norm13, norm24, norm13, norm24};
  return {
    .gammaRatios = ratios * scale,
    .enhancedContrast = params1->GetGrayscaleEnhancedContrast(),
  };
}

// Copyright (c) Microsoft Corporation, MIT license:
// https://github.com/microsoft/terminal/blob/main/src/renderer/atlas/dwrite_helpers.hlsl
[[nodiscard]]
inline Win32_DWrite_Blend_Table_For_Cpu
Win32_DWriteBlendTableForCpu(Win32_DWrite_Blend_Params params, Vector3<f32> color)
{
  f32 lightness = DotProduct(color, {0.30f, 0.59f, 0.11f});
  f32 contrast = params.enhancedContrast * Clamp(4 * (0.75f - lightness), 0.0f, 1.0f);
  f32 intensity = DotProduct(color, {0.25f, 0.5f, 0.25f});
  f32 correctionSlope = params.gammaRatios.x * intensity + params.gammaRatios.y;
  f32 correctionBase = params.gammaRatios.z * intensity + params.gammaRatios.w;

  Win32_DWrite_Blend_Table_For_Cpu table = {};
  for (u32 coverage = 0; coverage < 256; coverage += 1)
  {
    f32 alpha = coverage / 255.0f;
    alpha = alpha * (contrast + 1) / (alpha * contrast + 1);
    alpha += alpha * (1 - alpha) * (correctionSlope * alpha + correctionBase);
    table.alphas[coverage] = alpha;
  }
  return table;
}