#include "shapes.hh"
#include "d3dUtils.hh"
#include <DirectXMath.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

using namespace PD;
using namespace DirectX;

CubeMesh::CubeMesh() {

  float vertices[] = {
      // back face
      // pos normal tex
      -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
      1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
      1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // bottom-right
      1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
      -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
      -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // top-left
      // front face
      -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
      1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
      1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
      1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
      -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
      -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
      // left face
      -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
      -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-left
      -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
      -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
      -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
      -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
      // right face
      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
      1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
      1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-right
      1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
      1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
      1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
      // bottom face
      -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
      1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // top-left
      1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
      1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
      -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
      -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
      // top face
      -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
      1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
      1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // top-right
      1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
      -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
      -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f   // bottom-left
  };
  for (int i = 0; i < 288; i += 8) {
    value_type v;
    v.pos.x = vertices[i];
    v.pos.y = vertices[i + 1];
    v.pos.z = vertices[i + 2];
    v.normal.x = vertices[i + 3];
    v.normal.y = vertices[i + 4];
    v.normal.z = vertices[i + 5];
    v.tex.x = vertices[i + 6];
    v.tex.y = vertices[i + 7];
    vdata.push_back(v);
    //  indices.push_back()
  }
}
void CubeMesh::draw(ID3D11DeviceContext *context) {
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->Draw(36, 0);
}
PD::Model::Model(ID3D11Device *device, ID3D11DeviceContext *context, Path path)
    : context_(context), device_(device), path_(path) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      path.u8string(), aiProcess_Triangulate | aiProcess_GenSmoothNormals);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    spdlog::error("Assimp Open {} Failed", path.u8string());
    std::terminate();
  }
  processNode(scene->mRootNode, scene); // bfs the tree
}

void PD::Model::processNode(const aiNode *node, const aiScene *scene) {
  // 如果这个node有mesh信息，处理它的meshj
  for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
    // node->mMeshes[] 不存放实际的mesh信息，它存放index
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    this->meshes_.push_back(processMesh(mesh, scene));
  }
  //依次处理它的子节点，广度搜索
  for (uint32_t i = 0; i < node->mNumChildren; i++) {
    processNode(node->mChildren[i], scene);
  }
}
Model::Mesh PD::Model::processMesh(const aiMesh *mesh, const aiScene *scene) {

  std::vector<value_type> vdata;
  std::vector<index_type> idata;
  std::vector<Texture> textures;

  for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
    value_type v;
    std::memset(&v, 0, sizeof(v));
    v.pos = XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                     mesh->mVertices[i].z);
    if (mesh->mNormals) {
      v.normal = XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                          mesh->mNormals[i].z);
      auto normal = XMLoadFloat3(&v.normal);
      normal = XMVector3Normalize(normal);
      XMStoreFloat3(&v.normal, normal);
    }
    //只需要第一组texture坐标
    if (mesh->mTextureCoords[0]) {
      auto uv = mesh->mTextureCoords[0];
      v.tex = XMFLOAT2(uv->x, uv->y);
    }
    //我们先忽略tangent和bitangent
    vdata.push_back(v);
  }
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++)
      idata.push_back(face.mIndices[j]);
  }
  //处理材质
  // currently we ignore it
  return Mesh(vdata, idata, textures, device_, context_);
};
PD::details::Mesh::Mesh(const std::vector<value_type> &vertices,
                        const std::vector<index_type> &indices,
                        std::vector<Texture> textures, ID3D11Device *device,
                        ID3D11DeviceContext *context)
    : context_(context), device_(device), indices_size(indices.size()) {
  D3D11_BUFFER_DESC bufferDesc = {};
  bufferDesc.ByteWidth = uint32_t(vertices.size() * sizeof(value_type));
  bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  D3D11_SUBRESOURCE_DATA resourceData = {};
  resourceData.pSysMem = vertices.data();
  HRESULT hr;
  HR(device_->CreateBuffer(&bufferDesc, &resourceData, vbo_.GetAddressOf()));

  // IBO
  bufferDesc = {};
  bufferDesc.ByteWidth = uint32_t(indices.size() * sizeof(index_type));
  bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
  bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  resourceData = {};
  resourceData.pSysMem = indices.data();
  HR(device_->CreateBuffer(&bufferDesc, &resourceData, ibo_.GetAddressOf()));
}
void PD::Model::draw() const {
  for (const auto &mesh : meshes_) {
    mesh.draw();
  }
}
Model::~Model() {
  for (auto &mesh : meshes_) {
    SAFE_RELEASE(mesh.vbo_);
    SAFE_RELEASE(mesh.ibo_);
  }
}
void PD::details::Mesh::draw() const {

  context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  uint32_t offset[] = {0};
  static const uint32_t stride = sizeof(value_type);
  context_->IASetVertexBuffers(0, 1, vbo_.GetAddressOf(), &stride, offset);
  context_->IASetIndexBuffer(ibo_.Get(), DXGI_FORMAT_R32_UINT, 0);
  context_->DrawIndexed(indices_size, 0, 0);
}
