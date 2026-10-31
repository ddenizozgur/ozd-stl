cbuffer Frame : register(b0)
{
  float aspect;
  float imageAspect;
  float time;
};

Texture2D image : register(t0);
SamplerState linearWrap : register(s0);

struct Pixel
{
  float4 position : SV_Position;
  float2 uv : TEXCOORD;
};

static const float2 CORNERS[6] = {
  float2(-1, -1), float2(-1, 1), float2(1, 1),
  float2(-1, -1), float2(1, 1), float2(1, -1),
};

Pixel VSMain(uint vertexId : SV_VertexID)
{
  float2 corner = CORNERS[vertexId];

  float s, c;
  sincos(time * 0.3, s, c);
  float2 rotated = float2(corner.x * imageAspect * c - corner.y * s,
                          corner.x * imageAspect * s + corner.y * c);
  float scale = 0.5 + 0.4 * sin(time * 0.7);

  Pixel pixel;
  pixel.position = float4(rotated * scale / float2(aspect, 1), 0, 1);
  pixel.uv = (corner * float2(1, -1) * 0.5 + 0.5) * 2;
  return pixel;
}

float4 PSMain(Pixel pixel) : SV_Target
{
  return image.Sample(linearWrap, pixel.uv);
}
