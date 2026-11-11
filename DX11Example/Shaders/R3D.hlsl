static const float PI_F32 = 3.14159265;

static const uint PALETTE_COUNT = 23;
static const float3 PALETTE[PALETTE_COUNT] = {
  float3(0x6f, 0x67, 0x76) / 255.0,
  float3(0x9a, 0x9a, 0x97) / 255.0,
  float3(0xc5, 0xcc, 0xb8) / 255.0,
  float3(0x8b, 0x55, 0x80) / 255.0,
  float3(0xc3, 0x88, 0x90) / 255.0,
  float3(0xa5, 0x93, 0xa5) / 255.0,
  float3(0x66, 0x60, 0x92) / 255.0,
  float3(0x9a, 0x4f, 0x50) / 255.0,
  float3(0xc2, 0x8d, 0x75) / 255.0,
  float3(0x7c, 0xa1, 0xc0) / 255.0,
  float3(0x41, 0x6a, 0xa3) / 255.0,
  float3(0x8d, 0x62, 0x68) / 255.0,
  float3(0xbe, 0x95, 0x5c) / 255.0,
  float3(0x68, 0xac, 0xa9) / 255.0,
  float3(0x38, 0x70, 0x80) / 255.0,
  float3(0x6e, 0x69, 0x62) / 255.0,
  float3(0x93, 0xa1, 0x67) / 255.0,
  float3(0x6e, 0xaa, 0x78) / 255.0,
  float3(0x55, 0x70, 0x64) / 255.0,
  float3(0x9d, 0x9f, 0x7f) / 255.0,
  float3(0x7e, 0x9e, 0x99) / 255.0,
  float3(0x5d, 0x68, 0x72) / 255.0,
  float3(0x43, 0x34, 0x55) / 255.0,
};

// https://en.wikipedia.org/wiki/SRGB#Specification_of_the_transformation
float3 LinearToSrgb(float3 c) {
	c = saturate(c);
	return (c <= 0.0031308)
		? c * 12.92
		: 1.055 * pow(c, 1 / 2.4) - 0.055;
}

float3 DiffuseLambert(float3 albedoColor, float3 normal, float3 directionToLight)
{
  float3 brdf = albedoColor / PI_F32;
  float cosTheta = saturate(dot(normal, directionToLight));
  return brdf * cosTheta;
}

float3 DiffuseHalfLambert(float3 albedoColor, float3 normal, float3 directionToLight)
{
  float3 brdf = albedoColor / PI_F32;
  float cosTheta = dot(normal, directionToLight);
  float remapped = (cosTheta + 1) * 0.5;
  return brdf * remapped * remapped;
}

#define DIFFUSE_LAMBERT       0
#define DIFFUSE_HALF_LAMBERT  1

float3 DiffuseTerm(uint diffuseKind, float3 albedoColor, float3 normal, float3 directionToLight)
{
  float3 res = 0;
  switch (diffuseKind)
  {
  case DIFFUSE_LAMBERT:
    res = DiffuseLambert(albedoColor, normal, directionToLight);
    break;
  case DIFFUSE_HALF_LAMBERT:
    res = DiffuseHalfLambert(albedoColor, normal, directionToLight);
    break;
  }
  return res;
}

cbuffer Frame_Constants : register(b0)
{
  float4x4 viewProjectionMatrix;
  bool showAlbedo;
  float3 directionToLight;
  float lightIntensity;
  float3 lightColor;
  float ambient;
  uint diffuseKind;
};

cbuffer Mesh_Constants : register(b1)
{
  float4x4 modelMatrix;
};

StructuredBuffer<float3> positions : register(t0);
StructuredBuffer<float2> texcoords : register(t1);
StructuredBuffer<float3> normals : register(t2);
Texture2D albedo : register(t3);
SamplerState linearSampler : register(s0);

struct Pixel
{
  float4 position : SV_Position;
  float2 texcoord : TEXCOORD;
  float3 normal : NORMAL;
};

Pixel VSMain(uint vertexId : SV_VertexID)
{
  Pixel pixel;
  float4 worldPosition = mul(modelMatrix, float4(positions[vertexId], 1));
  pixel.position = mul(viewProjectionMatrix, worldPosition);
  pixel.texcoord = texcoords[vertexId];
  pixel.normal = mul(modelMatrix, float4(normals[vertexId], 0)).xyz;
  return pixel;
}

float4 PSMain(Pixel pixel, uint primitiveId : SV_PrimitiveID) : SV_Target
{
  float3 normal = normalize(pixel.normal);
  float3 albedoColor = albedo.Sample(linearSampler, pixel.texcoord).rgb;

  float3 sunIrradiancePerpendicular = lightColor * lightIntensity;
  float3 sunReflected = sunIrradiancePerpendicular * DiffuseTerm(diffuseKind, albedoColor, normal, directionToLight);

  float3 ambientBrdf = albedoColor / PI_F32;
  float3 ambientIrradiance = ambient * PI_F32;
  float3 ambientReflected = ambientBrdf * ambientIrradiance;

  float3 radianceToCamera = sunReflected + ambientReflected;

  float3 color = PALETTE[primitiveId % PALETTE_COUNT];
  if (showAlbedo)
  {
    color = LinearToSrgb(radianceToCamera);
  }
  return float4(color, 1);
}
