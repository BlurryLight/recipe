#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
namespace PD {
struct VertexPos {
  DirectX::XMFLOAT3 pos;
  static const D3D11_INPUT_ELEMENT_DESC inputLayout[1];
};

struct VertexPosColor {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT4 color;
  static const D3D11_INPUT_ELEMENT_DESC inputLayout[2];
};

struct VertexPosNormalColor {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT4 color;
  static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
};

struct VertexPosNormalTex {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT2 tex;
  static const D3D11_INPUT_ELEMENT_DESC inputLayout[3];
};
struct VertexPosNormalTangentTex {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT2 tangent_tex;
  DirectX::XMFLOAT2 tex;
  static const D3D11_INPUT_ELEMENT_DESC inputLayout[4];
};
} // namespace PD