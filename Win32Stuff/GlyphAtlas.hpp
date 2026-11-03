#pragma once

#include "../Base/ShelfPacker.hpp"
#include "../Vendor/rapidhash.h"
#include "DWrite.hpp"

// NOTE: check twice for garbage padding
struct Glyph_Atlas_Key
{
  IDWriteFontFace2 *face2;
  u32 index;
  f32 pixelSize;
};

struct Glyph_Atlas_Slot
{
  Vector2<u32> position; // top-left in the atlas
  Vector2<u32> size;
  Vector2<i32> offset; // top-left of coverage
};

[[nodiscard]] inline u64 GlyphAtlasKeyHash(Glyph_Atlas_Key key)
{
  return rapidhash(&key, sizeof(key));
}

[[nodiscard]]
inline bool GlyphAtlasKeyEqual(Glyph_Atlas_Key left, Glyph_Atlas_Key right)
{
  return left.face2 == right.face2 &&
         left.index == right.index &&
         left.pixelSize == right.pixelSize;
}

#define NAME Glyph_Atlas_Slots
#define KEY_TY Glyph_Atlas_Key
#define VAL_TY Glyph_Atlas_Slot
#define HASH_FN GlyphAtlasKeyHash
#define CMPR_FN GlyphAtlasKeyEqual
#include "../Vendor/verstable.h"

// NOTE: filtered sampling *might* need 1 to keep neighbors from bleeding in.
inline constexpr u32 GLYPH_ATLAS_PADDING = 0;

struct Glyph_Atlas
{
  Slice<u8> coverage; // Gray8, rows top to bottom with no padding
  Vector2<u32> size;
  Shelf_Packer packer;
  Glyph_Atlas_Slots slots;
};

[[nodiscard]]
inline Pair<Glyph_Atlas, bool> GlyphAtlasInit(Arena_Allocator *arena, Vector2<u32> size)
{
  auto coverage = ArenaPushSlice<u8>(arena, static_cast<usize>(size.width) * size.height);
  if (coverage.ptr == nullptr)
  {
    return {};
  }
  memset(coverage.ptr, 0, coverage.count);

  Glyph_Atlas atlas = {
    .coverage = coverage,
    .size = size,
    .packer = ShelfPackerInit(arena, size),
  };
  Glyph_Atlas_Slots_init(&atlas.slots);
  return {atlas, true};
}

// the coverage memory belongs to the arena given to GlyphAtlasInit
inline void GlyphAtlasRelease(Glyph_Atlas *atlas)
{
  Glyph_Atlas_Slots_cleanup(&atlas->slots);
}

inline void GlyphAtlasReset(Glyph_Atlas *atlas)
{
  Glyph_Atlas_Slots_clear(&atlas->slots);
  ShelfPackerReset(&atlas->packer);
  memset(atlas->coverage.ptr, 0, atlas->coverage.count);
}

// false when the glyph is bigger than the whole atlas.
[[nodiscard]]
inline Pair<Glyph_Atlas_Slot, bool>
Win32_DWriteGlyphFromAtlas(Glyph_Atlas *atlas, Win32_DWrite dwrite, Win32_DWrite_Font font, f32 pixelSize, u16 glyphIndex)
{
  Glyph_Atlas_Key key = {font.face2, glyphIndex, pixelSize};
  auto found = Glyph_Atlas_Slots_get(&atlas->slots, key);
  if (!Glyph_Atlas_Slots_is_end(found))
  {
    return {found.data->val, true};
  }

  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch););

  auto [glyph, glyphOk] = Win32_DWriteRasterizeGlyph(scratch.arena, dwrite, font, pixelSize, glyphIndex);
  if (!glyphOk)
  {
    return {};
  }

  Glyph_Atlas_Slot slot = {.size = glyph.size, .offset = glyph.offset};
  if (glyph.coverage.count != 0)
  {
    auto paddedSize = glyph.size + GLYPH_ATLAS_PADDING;
    auto placement = ShelfPackerPlace(&atlas->packer, paddedSize);
    if (!placement.second)
    {
      GlyphAtlasReset(atlas);
      placement = ShelfPackerPlace(&atlas->packer, paddedSize);
    }
    if (!placement.second)
    {
      return {};
    }

    slot.position = placement.first;
    for (u32 y = 0; y < glyph.size.height; y += 1)
    {
      auto row = static_cast<usize>(slot.position.y + y) * atlas->size.width + slot.position.x;
      memcpy(&atlas->coverage[row], &glyph.coverage[static_cast<usize>(y) * glyph.size.width], glyph.size.width);
    }
  }

  Glyph_Atlas_Slots_insert(&atlas->slots, key, slot);
  return {slot, true};
}
