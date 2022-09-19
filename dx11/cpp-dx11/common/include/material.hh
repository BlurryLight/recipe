#pragma once
#include <DirectXMath.h>
namespace PD {

struct PhongMaterial {
  DirectX::XMFLOAT4 ambient;
  DirectX::XMFLOAT4 diffuse;
  DirectX::XMFLOAT4 specular; // w is power
};

} // namespace PD