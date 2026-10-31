#pragma once

#include "GPU.hpp"
// D3D11 returns the same object for identical descs, so creating a preset twice is cheap.

//
// Blend
//
inline constexpr D3D11_BLEND_DESC DX11_BLEND_DESC_OPAQUE = {
    .RenderTarget = {{
        .BlendEnable = false,
        .SrcBlend = D3D11_BLEND_ONE,
        .DestBlend = D3D11_BLEND_ZERO,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_ONE,
        .DestBlendAlpha = D3D11_BLEND_ZERO,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
    }},
};

// color * alpha + destination * (1 - alpha)
inline constexpr D3D11_BLEND_DESC DX11_BLEND_DESC_ALPHA = {
    .RenderTarget = {{
        .BlendEnable = true,
        .SrcBlend = D3D11_BLEND_SRC_ALPHA,
        .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_ONE,
        .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
    }},
};

// color + destination * (1 - alpha)
inline constexpr D3D11_BLEND_DESC DX11_BLEND_DESC_PREMULTIPLIED_ALPHA = {
    .RenderTarget = {{
        .BlendEnable = true,
        .SrcBlend = D3D11_BLEND_ONE,
        .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_ONE,
        .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
    }},
};

// color + destination
inline constexpr D3D11_BLEND_DESC DX11_BLEND_DESC_ADDITIVE = {
    .RenderTarget = {{
        .BlendEnable = true,
        .SrcBlend = D3D11_BLEND_ONE,
        .DestBlend = D3D11_BLEND_ONE,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_ONE,
        .DestBlendAlpha = D3D11_BLEND_ONE,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
    }},
};

[[nodiscard]]
inline ID3D11BlendState *
DX11_CreateBlendState(DX11_GPU gpu, const D3D11_BLEND_DESC &desc)
{
  ID3D11BlendState *state = nullptr;
  gpu.device1->CreateBlendState(&desc, &state);
  return state;
}

//
// Rasterizer
//

// clockwise triangles are the front faces
inline constexpr D3D11_RASTERIZER_DESC DX11_RASTERIZER_DESC_CULL_BACK = {
    .FillMode = D3D11_FILL_SOLID,
    .CullMode = D3D11_CULL_BACK,
    .DepthClipEnable = true,
};

// draws both sides
inline constexpr D3D11_RASTERIZER_DESC DX11_RASTERIZER_DESC_NO_CULL = {
    .FillMode = D3D11_FILL_SOLID,
    .CullMode = D3D11_CULL_NONE,
    .DepthClipEnable = true,
};

inline constexpr D3D11_RASTERIZER_DESC DX11_RASTERIZER_DESC_WIREFRAME = {
    .FillMode = D3D11_FILL_WIREFRAME,
    .CullMode = D3D11_CULL_NONE,
    .DepthClipEnable = true,
};

[[nodiscard]]
inline ID3D11RasterizerState *
DX11_CreateRasterizerState(DX11_GPU gpu, const D3D11_RASTERIZER_DESC &desc)
{
  ID3D11RasterizerState *state = nullptr;
  gpu.device1->CreateRasterizerState(&desc, &state);
  return state;
}

//
// Depth
//
inline constexpr D3D11_DEPTH_STENCILOP_DESC DX11_STENCIL_OP_DESC_KEEP_ = {
    .StencilFailOp = D3D11_STENCIL_OP_KEEP,
    .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
    .StencilPassOp = D3D11_STENCIL_OP_KEEP,
    .StencilFunc = D3D11_COMPARISON_ALWAYS,
};

// no depth test or write
inline constexpr D3D11_DEPTH_STENCIL_DESC DX11_DEPTH_DESC_OFF = {
    .DepthEnable = false,
    .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
    .DepthFunc = D3D11_COMPARISON_ALWAYS,
    .FrontFace = DX11_STENCIL_OP_DESC_KEEP_,
    .BackFace = DX11_STENCIL_OP_DESC_KEEP_,
};

// nearer pixels win and write their depth
inline constexpr D3D11_DEPTH_STENCIL_DESC DX11_DEPTH_DESC_LESS = {
    .DepthEnable = true,
    .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
    .DepthFunc = D3D11_COMPARISON_LESS,
    .FrontFace = DX11_STENCIL_OP_DESC_KEEP_,
    .BackFace = DX11_STENCIL_OP_DESC_KEEP_,
};

// tests against depth without writing it
inline constexpr D3D11_DEPTH_STENCIL_DESC DX11_DEPTH_DESC_READ_ONLY = {
    .DepthEnable = true,
    .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO,
    .DepthFunc = D3D11_COMPARISON_LESS,
    .FrontFace = DX11_STENCIL_OP_DESC_KEEP_,
    .BackFace = DX11_STENCIL_OP_DESC_KEEP_,
};

[[nodiscard]]
inline ID3D11DepthStencilState *
DX11_CreateDepthState(DX11_GPU gpu, const D3D11_DEPTH_STENCIL_DESC &desc)
{
  ID3D11DepthStencilState *state = nullptr;
  gpu.device1->CreateDepthStencilState(&desc, &state);
  return state;
}

//
// Sampler
//
inline constexpr D3D11_SAMPLER_DESC DX11_SAMPLER_DESC_POINT_CLAMP = {
    .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
    .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
    .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
    .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
    .MaxAnisotropy = 1,
    .ComparisonFunc = D3D11_COMPARISON_NEVER,
    .MaxLOD = D3D11_FLOAT32_MAX,
};

inline constexpr D3D11_SAMPLER_DESC DX11_SAMPLER_DESC_POINT_WRAP = {
    .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
    .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
    .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
    .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
    .MaxAnisotropy = 1,
    .ComparisonFunc = D3D11_COMPARISON_NEVER,
    .MaxLOD = D3D11_FLOAT32_MAX,
};

inline constexpr D3D11_SAMPLER_DESC DX11_SAMPLER_DESC_LINEAR_CLAMP = {
    .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
    .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
    .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
    .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
    .MaxAnisotropy = 1,
    .ComparisonFunc = D3D11_COMPARISON_NEVER,
    .MaxLOD = D3D11_FLOAT32_MAX,
};

inline constexpr D3D11_SAMPLER_DESC DX11_SAMPLER_DESC_LINEAR_WRAP = {
    .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
    .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
    .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
    .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
    .MaxAnisotropy = 1,
    .ComparisonFunc = D3D11_COMPARISON_NEVER,
    .MaxLOD = D3D11_FLOAT32_MAX,
};

[[nodiscard]]
inline ID3D11SamplerState *
DX11_CreateSamplerState(DX11_GPU gpu, const D3D11_SAMPLER_DESC &desc)
{
  ID3D11SamplerState *state = nullptr;
  gpu.device1->CreateSamplerState(&desc, &state);
  return state;
}
