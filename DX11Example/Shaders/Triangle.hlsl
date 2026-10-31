#include "Common.hlsli"

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<float2> offsets : register(t1); // one per instance

struct Pixel
{
  float4 position : SV_Position;
  float4 color : COLOR;
};

Pixel VSMain(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
  Vertex vertex = vertices[vertexId];

  float s, c;
  sincos(time * 2, s, c);
  float2 rotated = float2(vertex.position.x * c - vertex.position.y * s,
                          vertex.position.x * s + vertex.position.y * c);
  float2 position = rotated * 0.15 + offsets[instanceId];

  Pixel pixel;
  pixel.position = float4(position.x / aspect, position.y, 0, 1);
  pixel.color = vertex.color;
  return pixel;
}

float4 PSMain(Pixel pixel) : SV_Target
{
  return pixel.color;
}
