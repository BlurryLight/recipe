#pragma once
#include "d3dx12.h"
#include <DirectXCollision.h>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <wrl.h>


using Microsoft::WRL::ComPtr;
namespace PD {
    namespace fs = std::filesystem;
    // dxd12book : define mesh
    // 一个MeshGeometry可能有多个Submesh构成，他们共用一个 vbo和ibo，只是使用其中的不同小段

    struct SubmeshGeometry {
        UINT IndexCount = 0;
        // 在多个mesh共用一个ibo/vbo时，需要指定baseIndex,并且无需改变index Buffer
        UINT StartIndexLocation = 0;
        INT BaseVertexLocation = 0;

        std::optional<DirectX::BoundingBox> Bounds;
    };

    struct MeshGeometry {
        std::string name;
        // ID3DBlob 类似于 std::any? 需要手动转型
        ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
        ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

        ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
        ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

        ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
        ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

        // some propoerties
        // 每个顶点之间的字节stride， sizeof(Vertex)
        UINT VertexBytesStride = 0;
        UINT VertexBufferByteSize = 0;
        DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
        UINT IndexBufferBytesSize = 0;
        std::unordered_map<std::string, struct SubmeshGeometry> DrawArgs;

        D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const;

        D3D12_INDEX_BUFFER_VIEW IndexBufferView() const;

        void DisposeUploaders();
    };

    struct SubMeshInfo {
        std::vector<DirectX::XMFLOAT3> mPoses;
        std::vector<uint32_t> mIndices32;
        std::vector<uint16_t> mIndices16;
        std::vector<DirectX::XMFLOAT3> mNormals;
        std::vector<DirectX::XMFLOAT2> mTexs;
        std::vector<DirectX::XMFLOAT3> mTangents;
        std::vector<DirectX::XMFLOAT3> mBiTangents;
    };
    std::vector<SubMeshInfo> LoadModelFromFile(fs::path modelPath);
}// namespace PD