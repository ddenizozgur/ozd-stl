#pragma once

#define CGLTF_IMPLEMENTATION
#include "../Vendor/cgltf.h"

#include "../Base/File.hpp"
#include "../Base/String.hpp"
#include "../Win32Stuff/WicImage.hpp"

[[nodiscard]]
inline Pair<Slice<u8>, bool>
GLTF_ImageBytesFrom(Arena_Allocator *arena, const cgltf_image *image, String gltfPath)
{
  if (image->buffer_view != nullptr)
  {
    auto view = image->buffer_view;
    auto bytes = const_cast<u8 *>(cgltf_buffer_view_data(view));
    return {{bytes, view->size}, true};
  }

  auto scratch = ScratchBegin();
  Defer(ScratchEnd(scratch, IsScratch(arena)););

  // copy because cgltf_decode_uri() overrides
  char *relative = CStringFrom(scratch.arena, StringFromC(image->uri));
  auto relativeCount = static_cast<i32>(cgltf_decode_uri(relative));
  String folder = StringBeforeLastInclusive(gltfPath, StringLit("/\\"));
  String path = StringFmt(scratch.arena, "%.*s%.*s", StringArgs(folder), relativeCount, relative);
  return Win32_ReadEntireFile(arena, path);
}

[[nodiscard]]
inline Pair<Image, bool>
GLTF_LoadAlbedo(Arena_Allocator *arena, const cgltf_primitive *primitive,
                Image_Format decodeAs, String gltfPath)
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

  auto [bytes, bytesOk] = GLTF_ImageBytesFrom(scratch.arena, texture->image, gltfPath);
  if (!bytesOk)
  {
    return {};
  }
  return Win32_WicDecodeImageAs(arena, bytes, decodeAs);
}

[[nodiscard]]
inline Slice<Vector2<f32>>
GLTF_LoadTexcoords(Arena_Allocator *arena, const cgltf_primitive *primitive, i32 set)
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

[[nodiscard]]
inline Slice<Vector3<f32>>
GLTF_LoadPositions(Arena_Allocator *arena, const cgltf_primitive *primitive)
{
  auto accessor = cgltf_find_accessor(primitive, cgltf_attribute_type_position, 0);
  if (accessor == nullptr)
  {
    return {};
  }
  auto positions = ArenaPushSlice<Vector3<f32>>(arena, accessor->count);
  cgltf_accessor_unpack_floats(accessor, positions.ptr->data, positions.count * 3);
  return positions;
}

[[nodiscard]]
inline Slice<Vector3<f32>>
GLTF_LoadNormals(Arena_Allocator *arena, const cgltf_primitive *primitive)
{
  auto accessor = cgltf_find_accessor(primitive, cgltf_attribute_type_normal, 0);
  if (accessor == nullptr)
  {
    return {};
  }
  auto normals = ArenaPushSlice<Vector3<f32>>(arena, accessor->count);
  cgltf_accessor_unpack_floats(accessor, normals.ptr->data, normals.count * 3);
  return normals;
}

[[nodiscard]]
inline Slice<u32>
GLTF_LoadIndices(Arena_Allocator *arena, const cgltf_primitive *primitive)
{
  auto accessor = primitive->indices;
  if (accessor == nullptr)
  {
    return {};
  }
  auto indices = ArenaPushSlice<u32>(arena, accessor->count);
  cgltf_accessor_unpack_indices(accessor, indices.ptr, sizeof(u32), indices.count);
  return indices;
}