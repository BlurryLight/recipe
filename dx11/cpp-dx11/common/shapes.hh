#pragma once
#include "vertexLayout.hh"
#include <DirectXMath.h>
#include <d3d11.h>
#include <vector>

namespace PD {
struct Mesh {
  virtual void draw(ID3D11DeviceContext *) = 0;
};
struct CubeMesh : public Mesh {
  using value_type = VertexPosNormalTex;
  using index_type = uint32_t;
  CubeMesh();
  std::vector<VertexPosNormalTex> vdata;
  std::vector<uint32_t> indices;
  void draw(ID3D11DeviceContext *context) override;
};

// struct SphereMesh {
//   using value_type = VertexPosNormalTex;
//   using index_type = uint32_t;
//   SphereMesh();
//   std::vector<VertexPosNormalTex> vdata;
//   std::vector<uint32_t> indices;
// };
// struct QuadMesh {
//   using value_type = VertexPosNormalTex;
//   using index_type = uint32_t;
//   QuadMesh();
//   std::vector<VertexPosNormalTex> vdata;
//   std::vector<uint32_t> indices;
// };
// struct ScreenQuadMesh {
//   using value_type = VertexPosTex;
//   using index_type = uint32_t;
//   ScreenQuadMesh();
//   std::vector<VertexPosTex> vdata;
// };
} // namespace PD