//
// Created by zhong on 2021/3/27.
//

#pragma once
#include "d3dx12.h"
#include <D3Dcompiler.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <iostream>
#include <unordered_map>
#include <windows.h>
#include <wrl.h>

// com release
#define SAFE_RELEASE(p)                                                                                                \
    {                                                                                                                  \
        if ((p)) {                                                                                                     \
            (p)->Release();                                                                                            \
            (p) = nullptr;                                                                                             \
        }                                                                                                              \
    }
#define HR_RETURN(op)                                                                                                  \
    do {                                                                                                               \
        HRESULT hr__ = (op);                                                                                           \
        if (FAILED(hr__) {                    \
            DxTrace(__FILEW__, __LINE__, hr__, L#op); \
            return false;                             \
        }                                                                                                              \
    } while (0)

#define HR(op)                                                                                                         \
    do {                                                                                                               \
        HRESULT hr__ = (op);                                                                                           \
        if (FAILED(hr__)) {                                                                                            \
            DxTrace(__FILEW__, __LINE__, hr__, L#op);                                                                  \
            assert(0);                                                                                                 \
        }                                                                                                              \
    } while (0)

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) HR(x)
#endif


using Microsoft::WRL::ComPtr;
namespace PD {

    struct Noncopyable {
        Noncopyable() = default;
        Noncopyable(const Noncopyable &) = delete;
        Noncopyable &operator=(const Noncopyable &) = delete;
    };
    class d3dUtils {};

    void DxTrace(const wchar_t *file, unsigned long line, HRESULT hr, const wchar_t *proc);

    std::string utf16_to_utf8_windows(std::wstring const &utf16s);
    std::wstring utf8_to_utf16_windows(std::string const &utf8s);
    // https://stackoverflow.com/questions/72556894/directx-12-ultimate-graphics-sample-generates-a-d3d12-cbv-invalid-resource-err
    // see D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT 256
    // ConstantBuffer must be  256-bytes alignment
    inline uint32_t CalcConstantBufferBytesSize(uint32_t byteSize) { return (byteSize + 0xFF) & ~0xFF; }
    HRESULT CreateShaderFromFile(std::wstring_view csoFileNameInOut, std::wstring_view hlslPath, std::string entryPoint,
                                 std::string ShaderModel, ID3DBlob **ppBlobOut, bool force_compile = false,
                                 const D3D_SHADER_MACRO *defines = nullptr);

    ComPtr<ID3DBlob> CompileShader(const std::wstring &filename, const D3D_SHADER_MACRO *defines,
                                   const std::string &entrypoint, const std::string &target);

    ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList,
                                               const void *initData, uint64_t byteSize,
                                               ComPtr<ID3D12Resource> &outUploader);

    struct SubmeshGeometry {
        UINT IndexCount = 0;
        // 在多个mesh共用一个ibo/vbo时，需要指定baseIndex,并且无需改变index Buffer
        UINT StartIndexLocaion = 0;
        INT BaseVertexLocation = 0;

        DirectX::BoundingBox Bounds;
    };

    // dxd12book : define mesh
    // 一个MeshGeometry可能有多个Submesh构成，他们共用一个 vbo和ibo，只是使用其中的不同小段
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
        UINT IndexbufferBytesSize = 0;
        std::unordered_map<std::string, struct SubmeshGeometry> DrawArgs;

        D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const {
            D3D12_VERTEX_BUFFER_VIEW vbv;
            vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
            vbv.StrideInBytes = VertexBytesStride;
            vbv.SizeInBytes = VertexBufferByteSize;
            return vbv;
        }

        D3D12_INDEX_BUFFER_VIEW IndexBufferView() const {
            D3D12_INDEX_BUFFER_VIEW ibv;
            ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
            ibv.Format = IndexFormat;
            ibv.SizeInBytes = IndexbufferBytesSize;
            return ibv;
        }

        void DisposeUploaders() {
            SAFE_RELEASE(VertexBufferUploader);
            SAFE_RELEASE(IndexBufferUploader);
        }
    };
}// namespace PD