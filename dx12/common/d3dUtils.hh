//
// Created by zhong on 2021/3/27.
//

#pragma once
#include "MeshGeometry.hh"
#include "d3dx12.h"
#include <D3Dcompiler.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d12.h>
#include <dxgi1_4.h>
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
            std::abort();                                                                                                 \
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

    /**
     * @brief 一个辅助函数，创建一个位于DefaultHeap的Buffer对象，数据先到Upload堆，再到Default堆
     * 
     * @param device 
     * @param cmdList 
     * @param initData 位于CPU上的数据指针
     * @param byteSize 数据字节数
     * @param outUploader 会返回一个位于Upload堆的对象，需要维持生命周期到ExecuteCommandList
     * @return ComPtr<ID3D12Resource> 
     */
    ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList,
                                               const void *initData, uint64_t byteSize,
                                               ComPtr<ID3D12Resource> &outUploader);

    void D3D12ClearDebugObjectName(ID3D12DeviceChild *resource);

    void D3D12SetDebugObjectName(ID3D12DeviceChild *resource, std::string_view name);

    void DXGISetDebugObjectName(IDXGIObject *object, std::string_view name);

    void DXGIClearDebugObjectName(IDXGIObject *object);


}// namespace PD