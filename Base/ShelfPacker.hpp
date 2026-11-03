#pragma once

#include "ArenaAllocator.hpp"
#include "FixedList.hpp"
#include "Math.hpp"

inline constexpr u32 SHELF_HEIGHT_STEP = 8;

struct Shelf
{
  u32 y;
  u32 height;
  u32 nextX;
};

struct Shelf_Packer
{
  Vector2<u32> size;
  Fixed_List<Shelf> shelves;
  u32 nextShelfY;
};

[[nodiscard]]
inline Shelf_Packer ShelfPackerInit(Arena_Allocator *arena, Vector2<u32> size)
{
  auto shelfStorage = ArenaPushSlice<Shelf>(arena, size.height / SHELF_HEIGHT_STEP);
  return {.size = size, .shelves = FixedListInit(shelfStorage)};
}

inline void ShelfPackerReset(Shelf_Packer *packer)
{
  FixedListClear(&packer->shelves);
  packer->nextShelfY = 0;
}

// returns the top-left of the rectangle, false when it doesn't fit
[[nodiscard]]
inline Pair<Vector2<u32>, bool> ShelfPackerPlace(Shelf_Packer *packer, Vector2<u32> size)
{
  // the shortest shelf that has room
  Shelf *best = nullptr;
  for (auto &shelf : packer->shelves)
  {
    bool fits = size.width <= packer->size.width - shelf.nextX &&
                size.height <= shelf.height;
    if (fits && (best == nullptr || shelf.height < best->height))
    {
      best = &shelf;
    }
  }

  // a shelf more than 1.5 times taller than a new one wastes space, the new one is better when there is room
  auto shelfHeight = static_cast<u32>(AlignForward(Maximum(size.height, 1u), SHELF_HEIGHT_STEP));
  bool wasteful = best != nullptr && best->height * 2 > shelfHeight * 3;
  bool roomForShelf = shelfHeight <= packer->size.height - packer->nextShelfY &&
                      size.width <= packer->size.width &&
                      packer->shelves.count < packer->shelves.backingBuffer.count;
  if ((best == nullptr || wasteful) && roomForShelf)
  {
    Shelf shelf = {packer->nextShelfY, shelfHeight, 0};
    FixedListAppend(&packer->shelves, {&shelf, 1});
    best = &packer->shelves[packer->shelves.count - 1];
    packer->nextShelfY += shelfHeight;
  }
  if (best == nullptr)
  {
    return {};
  }

  Vector2<u32> position = {best->nextX, best->y};
  best->nextX += size.width;
  return {position, true};
}
