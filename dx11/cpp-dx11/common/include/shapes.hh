#pragma once
#include "vertexLayout.hh"
#include <DirectXMath.h>
#include <d3d11.h>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

struct aiMesh;
struct aiScene;
struct aiNode;
namespace PD {
struct IMesh { // Non-copyable
  IMesh() {}
  IMesh(const IMesh &) = delete;
  IMesh &operator=(const IMesh &) = delete;
  virtual void draw() const = 0;
  virtual ~IMesh() {}
};

namespace details {

struct Texture { // currently its a empty impelmentation
  std::string type;
};
struct Mesh {
  using value_type = VertexPosNormalTex;
  using index_type = uint32_t;

  ~Mesh(){};
  ComPtr<ID3D11Buffer> vbo_ = nullptr;
  ComPtr<ID3D11Buffer> ibo_ = nullptr;
  ID3D11DeviceContext *context_ = nullptr;
  ID3D11Device *device_ = nullptr;
  Mesh(const std::vector<value_type> &vertices,
       const std::vector<index_type> &indices, std::vector<Texture> textures,
       ID3D11Device *device, ID3D11DeviceContext *context);
  void draw() const;
  uint32_t indices_size = 0;
  // using value_type = VertexPosNormalTex;
  // using index_type = uint32_t;
  // std::vector<VertexPosNormalTex> vdata;
  // std::vector<uint32_t> indices;
};
} // namespace details
struct Model : IMesh {
  using Path = std::filesystem::path;
  using Texture = details::Texture;
  using Mesh = details::Mesh;
  using value_type = Mesh::value_type;
  using index_type = Mesh::index_type;
  Model(ID3D11Device *device, ID3D11DeviceContext *context, Path path);
  Model() = default;
  void draw() const override;
  ~Model();

private:
  std::unordered_map<std::string, Texture> loaded_textures_;
  std::vector<Mesh> meshes_;
  Mesh processMesh(const aiMesh *, const aiScene *);
  void processNode(const aiNode *, const aiScene *);
  ID3D11DeviceContext *context_ = nullptr;
  ID3D11Device *device_ = nullptr;
  Path path_;
};
struct CubeMesh : public IMesh {
  using value_type = VertexPosNormalTex;
  using index_type = uint32_t;
  ~CubeMesh() {
    assert(vbo_);
    vbo_->Release();
    vbo_ = nullptr;
  }
  CubeMesh(ID3D11Device *, ID3D11DeviceContext *);
  std::vector<VertexPosNormalTex> vdata;
  ID3D11Buffer *vbo_ = nullptr;
  ID3D11Device *device_ = nullptr;
  ID3D11DeviceContext *context_ = nullptr;
  void draw() const override;
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