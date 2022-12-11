//
// Created by zhong on 2021/3/27.
//

#include "d3dUtils.hh"
#include <filesystem>
#include <iostream>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/xchar.h>
#include <spdlog/spdlog.h>
namespace fs = std::filesystem;
using namespace PD;
void PD::DxTrace(const wchar_t *file, unsigned long line, HRESULT hr, const wchar_t *proc) {

    char *output;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                   hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (char *) &output, 0, NULL);
    std::wcerr << "file:" << file << "line:" << line << ", " << proc << std::endl;
    std::printf("Error Msg: %s", output);
}

std::string PD::utf16_to_utf8_windows(std::wstring const &utf16s) {
    // https://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
    int count = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *) (utf16s.c_str()), (int) utf16s.size(), NULL, 0, NULL, NULL);
    std::string res;
    res.resize(count);
    int flag =
            WideCharToMultiByte(CP_UTF8, 0, (wchar_t *) utf16s.c_str(), utf16s.size(), res.data(), count, NULL, NULL);
    if (!flag) {
        std::cerr << "converted u16 to utf8 failed" << std::endl;
        return "";
    }
    return res;
}

std::wstring PD::utf8_to_utf16_windows(std::string const &utf8) {
    // 见 https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
    // If the provided size does not include a terminating null character, the resulting Unicode string is not null-terminated,
    // and the returned length does not include this character.
    // 注意要传入准确地utf8.size(),size()是不包含\0的长度
    // 否则WindowsAPI 会计入包含 \0的长度
    // 因为结果填充到std::wstring里，std::wstring自带补0，所以WinAPI返回的字符串应该为不含0的字符
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), (int) utf8.size(), NULL, 0);
    std::wstring res;
    res.resize(count);
    int flag = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), (int) utf8.size(),
                                   (wchar_t *) (res.data()), count);
    if (!flag) {
        std::cerr << "converted u8 to utf16 failed" << std::endl;
        return L"";
    }
    return res;
}

HRESULT PD::CreateShaderFromFile(std::wstring_view csoFileNameInOut, std::wstring_view hlslPath, std::string entryPoint,
                                 std::string ShaderModel, ID3DBlob **ppBlobOut, bool force_compile,
                                 const D3D_SHADER_MACRO *defines) {
    HRESULT hr = S_OK;
    // 当hlsl比cso新的时候必须重编
    force_compile = force_compile || (fs::last_write_time(csoFileNameInOut) < fs::last_write_time(hlslPath));
    if (!force_compile && !csoFileNameInOut.empty() &&
        (D3DReadFileToBlob(csoFileNameInOut.data(), ppBlobOut) == S_OK)) {
        return hr;
    } else {
        DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        dwShaderFlags |= D3DCOMPILE_DEBUG;
        dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        ID3DBlob *errorBlob = nullptr;
        hr = D3DCompileFromFile(hlslPath.data(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.data(),
                                ShaderModel.data(), dwShaderFlags, 0, ppBlobOut, &errorBlob);
        if (FAILED(hr)) {
            if (errorBlob) { OutputDebugStringA((const char *) (errorBlob->GetBufferPointer())); }
            SAFE_RELEASE(errorBlob);
            return hr;
        }
        if (!csoFileNameInOut.empty()) {
            // default: don't override existed cso
            // force_compile: override it
            hr = D3DWriteBlobToFile(*ppBlobOut, csoFileNameInOut.data(), force_compile);
        }
        return hr;
    }
}
// d3d12book same api
ComPtr<ID3DBlob> PD::CompileShader(const std::wstring &filename, const D3D_SHADER_MACRO *defines,
                                   const std::string &entrypoint, const std::string &target) {
    ComPtr<ID3DBlob> Res = nullptr;

    auto entrypointL = utf8_to_utf16_windows(entrypoint);
    auto targetL = utf8_to_utf16_windows(target);
    // std::wstring csoPath = fmt::format(L"{}_{}_{}.cso", filename, entrypointL, targetL);
    auto csoPath = filename + L"_" + entrypointL + L"_" + targetL + L".cso";
    std::wcout << csoPath << std::endl;
    CreateShaderFromFile(csoPath, filename, entrypoint, target, Res.GetAddressOf(), true, defines);
    return Res;
}

ComPtr<ID3D12Resource> PD::CreateDefaultBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList,
                                               const void *initData, uint64_t byteSize,
                                               ComPtr<ID3D12Resource> &outUploader) {
    ComPtr<ID3D12Resource> defaultBuffer;

    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto defaultResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    // 创建了DefaultBuffer，但是内部是空的
    HR(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &defaultResourceDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&defaultBuffer)));

    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    assert(!outUploader);
    HR(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadResourceDesc,
                                       D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&outUploader)));

    assert(defaultBuffer);
    assert(outUploader);
    D3D12_SUBRESOURCE_DATA subResourceData{};
    subResourceData.pData = initData;
    // 对于buffer，这两个属性都应该等于字节数
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = byteSize;

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                                      D3D12_RESOURCE_STATE_COPY_DEST));
    // 首先从CPU端拷贝到uploader里，再拷贝到default heap里
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), outUploader.Get(), 0, 0, 1, &subResourceData);
    cmdList->ResourceBarrier(1,
                             &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                   D3D12_RESOURCE_STATE_GENERIC_READ));
    return defaultBuffer;
}
