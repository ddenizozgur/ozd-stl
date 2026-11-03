cbuffer UI_Constants : register(b0)
{
  float4 gammaRatios;
  float2 screenSize;
  float enhancedContrast;
};

static const uint KIND_RECT = 0;
static const uint KIND_GLYPH = 1;
static const uint KIND_IMAGE = 2;

struct Instance
{
  float2 position; // top-left in pixels
  float2 size;
  float2 atlasPosition;
  uint color; // straight alpha
  uint kind;
};

StructuredBuffer<Instance> instances : register(t0);
Texture2D<float> atlas : register(t1);
Texture2D image : register(t2);
SamplerState linearSampler : register(s0);

struct Pixel
{
  float4 position : SV_Position;
  nointerpolation float2 screenToAtlas : ATLAS;
  nointerpolation float4 color : COLOR;
  nointerpolation uint kind : KIND;
  float2 uv : TEXCOORD;
};

float4 UnpackRGBA8(uint packed)
{
  return float4(packed & 0xFF, (packed >> 8) & 0xFF, (packed >> 16) & 0xFF, packed >> 24) / 255.0;
}

Pixel VSMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
  Instance instance = instances[instanceId];

  // TL, TR, BL, BR
  float2 corner = float2(vertexId & 1, vertexId >> 1);
  float2 pixelPosition = instance.position + corner * instance.size;

  Pixel pixel;
  pixel.position = float4(pixelPosition / screenSize * float2(2, -2) + float2(-1, 1), 0, 1);
  pixel.screenToAtlas = instance.atlasPosition - instance.position;
  pixel.color = UnpackRGBA8(instance.color);
  pixel.kind = instance.kind;
  pixel.uv = corner;
  return pixel;
}

// Copyright (c) Microsoft Corporation, MIT license:
// https://github.com/microsoft/terminal/blob/main/src/renderer/atlas/dwrite_helpers.hlsl
float GammaCorrectedAlpha(float coverage, float3 color)
{
  float lightness = dot(color, float3(0.30, 0.59, 0.11));
  float contrast = enhancedContrast * saturate(4 * (0.75 - lightness));
  float intensity = dot(color, float3(0.25, 0.5, 0.25));

  float alpha = coverage * (contrast + 1) / (coverage * contrast + 1);
  float correctionSlope = gammaRatios.x * intensity + gammaRatios.y;
  float correctionBase = gammaRatios.z * intensity + gammaRatios.w;
  return alpha + alpha * (1 - alpha) * (correctionSlope * alpha + correctionBase);
}

float4 PSMain(Pixel pixel) : SV_Target
{
  float4 color = float4(pixel.color.rgb * pixel.color.a, pixel.color.a);
  if (pixel.kind == KIND_GLYPH)
  {
    int2 texel = int2(pixel.position.xy + pixel.screenToAtlas);
    float coverage = atlas.Load(int3(texel, 0));
    return color * GammaCorrectedAlpha(coverage, pixel.color.rgb);
  }
  if (pixel.kind == KIND_IMAGE)
  {
    return image.Sample(linearSampler, pixel.uv) * color;
  }
  return color;
}
