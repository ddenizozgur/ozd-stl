struct Vertex
{
  float2 position;
  float4 color;
};

cbuffer Frame : register(b0)
{
  float aspect; // width / height
  float time;   // seconds
};
