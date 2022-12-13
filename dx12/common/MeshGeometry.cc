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
using SubmeshArrays = std::vector<SubMeshInfo>;

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

SubMeshInfo processMesh(const aiMesh *mesh, const aiScene *scene);
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

SubMeshInfo processMesh(const aiMesh *mesh, const aiScene *scene) {
    SubMeshInfo info;
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        info.mPoses.push_back(XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
        if (mesh->mNormals) {
            Vector3 normal = XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            normal.Normalize();
            info.mNormals.push_back(normal);
        }
        //暂时只需要第一组texture坐标
        if (mesh->mTextureCoords[0]) {
            auto uv = mesh->mTextureCoords[0][i];
            info.mTexs.push_back(DirectX::XMFLOAT2(uv.x, uv.y));
        }
        info.mTangents.push_back(XMFLOAT3(
            mesh->mTangents[i].x,
            mesh->mTangents[i].y,
            mesh->mTangents[i].z
        ));
        info.mBiTangents.push_back(XMFLOAT3(
            mesh->mBitangents[i].x,
            mesh->mBitangents[i].y,
            mesh->mBitangents[i].z
        ));
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            uint32_t index = face.mIndices[j];
            info.mIndices32.push_back(index);

            assert(index <= std::numeric_limits<uint16_t>::max());

            info.mIndices16.push_back(static_cast<uint16_t>(index));
        }
    }
    return info;
};

std::vector<SubMeshInfo> PD::LoadModelFromFile(fs::path ModelPath) {
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
