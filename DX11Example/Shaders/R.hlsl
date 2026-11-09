cbuffer Constants : register(b0)
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
  float2 dstPosition;
  float2 dstSize;
  float2 srcPosition;
  float2 srcSize;
  uint color;
  uint kind;
  float cornerRadius;
  float borderThickness;
};

StructuredBuffer<Instance> instances : register(t0);
Texture2D<float> atlas : register(t1);
Texture2D image : register(t2);
SamplerState linearSampler : register(s0);

struct Pixel
{
  float4 position : SV_Position;
  float2 texel : TEXEL;
  nointerpolation float4 color : COLOR;
  nointerpolation uint kind : KIND;
  nointerpolation float2 center : CENTER;
  nointerpolation float2 halfSize : HALFSIZE;
  nointerpolation float cornerRadius : RADIUS;
  nointerpolation float borderThickness : BORDER;
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
  float2 pixelPosition = instance.dstPosition + corner * instance.dstSize;

  Pixel pixel;
  pixel.position = float4(pixelPosition / screenSize * float2(2, -2) + float2(-1, 1), 0, 1);
  pixel.texel = instance.srcPosition + corner * instance.srcSize;
  pixel.color = UnpackRGBA8(instance.color);
  pixel.kind = instance.kind;
  pixel.halfSize = instance.dstSize * 0.5;
  pixel.center = instance.dstPosition + pixel.halfSize;
  pixel.cornerRadius = min(instance.cornerRadius, min(pixel.halfSize.x, pixel.halfSize.y));
  pixel.borderThickness = instance.borderThickness;
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

// Inigo Quilez's sdRoundedBox: https://iquilezles.org/articles/distfunctions2d/
float SdRoundedBox(float2 p, float2 halfSize, float radius)
{
  float2 q = abs(p) - halfSize + radius;
  return min(max(q.x, q.y), 0) + length(max(q, 0)) - radius;
}

float4 PSMain(Pixel pixel) : SV_Target
{
  float4 color = float4(pixel.color.rgb * pixel.color.a, pixel.color.a);
  if (pixel.kind == KIND_GLYPH)
  {
    float coverage = atlas.Load(int3(int2(pixel.texel), 0));
    return color * GammaCorrectedAlpha(coverage, pixel.color.rgb);
  }

  if (pixel.kind == KIND_IMAGE)
  {
    float width, height;
    image.GetDimensions(width, height);
    color *= image.Sample(linearSampler, pixel.texel / float2(width, height));
  }

  float distance = SdRoundedBox(pixel.position.xy - pixel.center, pixel.halfSize, pixel.cornerRadius);
  if (pixel.borderThickness > 0)
  {
    float halfBorder = pixel.borderThickness * 0.5;
    distance = abs(distance + halfBorder) - halfBorder;
  }
  return color * saturate(0.5 - distance);
}
