#pragma once

#include <DirectXMath.h>

namespace PD {

struct DirectionalLight {
  DirectX::XMFLOAT4 color;
  DirectX::XMFLOAT3 direction;
  float padding;
};
}; // namespace PD
