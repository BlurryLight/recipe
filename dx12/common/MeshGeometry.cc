#include "MeshGeometry.hh"
#include <SimpleMath.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <spdlog/spdlog.h>

using DirectX::XMFLOAT3;
using namespace DirectX::SimpleMath;

using namespace PD;
using SubmeshArrays = std::vector<PD::MeshGeometry::SubMeshInfo>;

D3D12_VERTEX_BUFFER_VIEW PD::MeshGeometry::VertexBufferView() const {
    assert(VertexBytesStride > 0);
    assert(VertexBufferByteSize > 0);
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
    vbv.StrideInBytes = VertexBytesStride;
    vbv.SizeInBytes = VertexBufferByteSize;
    return vbv;
}
D3D12_INDEX_BUFFER_VIEW PD::MeshGeometry::IndexBufferView() const {
    assert(IndexBufferBytesSize > 0);
    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
    ibv.Format = IndexFormat;
    ibv.SizeInBytes = IndexBufferBytesSize;
    return ibv;
}
void PD::MeshGeometry::DisposeUploaders() {
    VertexBufferUploader.Reset();
    IndexBufferUploader.Reset();
}

PD::MeshGeometry::SubMeshInfo processMesh(const aiMesh *mesh, const aiScene *scene);
void processNode(SubmeshArrays &faces, const aiNode *node, const aiScene *scene) {
    // 如果这个node有mesh信息，处理它的mesh
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        // node->mMeshes[] 不存放实际的mesh信息，它存放index
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        // meshes_.push_back(processMesh(mesh, scene));
        std::string subMeshName = fmt::format("part_{}", i);
        faces.push_back(processMesh(mesh, scene));
    }
    //依次处理它的子节点，广度搜索
    for (uint32_t i = 0; i < node->mNumChildren; i++) { processNode(faces, node->mChildren[i], scene); }
}

PD::MeshGeometry::SubMeshInfo processMesh(const aiMesh *mesh, const aiScene *scene) {


    PD::MeshGeometry::SubMeshInfo info;
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        info.poses.push_back(XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
        if (mesh->mNormals) {
            Vector3 normal = XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            normal.Normalize();
        }
        //暂时只需要第一组texture坐标
        if (mesh->mTextureCoords[0]) {
            auto uv = mesh->mTextureCoords[0][i];
            // v.tex = XMFLOAT2(uv.x, uv.y);
        }
        //我们先忽略tangent和bitangent
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            uint32_t index = face.mIndices[j];
            info.indices.push_back(index);
            // TODO: add a wanring about it
            info.indices_16.push_back(static_cast<uint16_t>(index));
        }
    }
    return info;
};

std::vector<PD::MeshGeometry::SubMeshInfo> PD::MeshGeometry::LoadFromFile(fs::path ModelPath) {
    const auto pathStr = fs::absolute(ModelPath).u8string();
    spdlog::info("Loading Model File from {}", pathStr);
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(pathStr, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                              aiProcess_GenUVCoords | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        spdlog::error("Assimp Open {} Failed", pathStr);
        std::abort();
    }

    SubmeshArrays res;
    processNode(res, scene->mRootNode, scene);// bfs the tree
    return res;
}
