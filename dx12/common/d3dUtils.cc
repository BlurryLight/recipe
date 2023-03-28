//
// Created by zhong on 2021/3/27.
//

#include "d3dUtils.hh"
#include <DDSTextureLoader12.h>
#include <array>
#include <filesystem>
#include <iostream>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/xchar.h>
#include <spdlog/spdlog.h>
#include <stb_image/stb_image.h>
using namespace PD;
void PD::DxTrace(const wchar_t *file, unsigned long line, HRESULT hr, const wchar_t *proc) {

    char *output;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                   hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (char *) &output, 0, NULL);
    std::wcerr << "file:" << file << "line:" << line << ", " << proc << std::endl;
    std::printf("Error Msg: %s", output);
    LocalFree(output);
}

std::string PD::utf16_to_utf8_windows(std::wstring const &utf16s) {
    // https://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
    if (utf16s.empty()) return {};
    int count = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *) (utf16s.c_str()), (int) utf16s.size(), NULL, 0, NULL, NULL);
    std::string res;
    res.resize(count);
    int flag =
            WideCharToMultiByte(CP_UTF8, 0, (wchar_t *) utf16s.c_str(), utf16s.size(), res.data(), count, NULL, NULL);
    if (!flag) {
        DxTrace(__FILEW__, __LINE__, GetLastError(), L"utf16_to_utf8_windows");
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
    if (utf8.empty()) return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), (int) utf8.size(), NULL, 0);
    std::wstring res;
    res.resize(count);
    int flag = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), (int) utf8.size(),
                                   (wchar_t *) (res.data()), count);
    if (!flag) {
        DxTrace(__FILEW__, __LINE__, GetLastError(), L"utf16_to_utf8_windows");
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
        std::string version_str = ShaderModel.substr(ShaderModel.size() - 3);// ps_5_1
        version_str[1] = '.';
        float version_num = std::stof(version_str);
        if (version_num > 5.0) { dwShaderFlags |= D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES; }

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
    std::string prefix = "";
    auto def = defines;
    while (def != NULL && def->Name != NULL) {
        prefix += def->Name;
        prefix += "_";
        prefix += def->Definition;
        prefix += "_";
        def++;
    }
    auto csoPath = filename + L"_" + utf8_to_utf16_windows(prefix) + L"_" + entrypointL + L"_" + targetL + L".cso";
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
                                       D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&defaultBuffer)));

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

    // cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON,
    //                                                                   D3D12_RESOURCE_STATE_COPY_DEST));
    // 首先从CPU端拷贝到uploader里，再拷贝到default heap里
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), outUploader.Get(), 0, 0, 1, &subResourceData);
    cmdList->ResourceBarrier(1,
                             &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                   D3D12_RESOURCE_STATE_GENERIC_READ));
    return defaultBuffer;
}

void PD::D3D12SetDebugObjectName(ID3D12DeviceChild*resource, std::string_view name) {
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT) name.size(), name.data());
}

void PD::D3D12ClearDebugObjectName(ID3D12DeviceChild*resource) {
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
}

void PD::DXGISetDebugObjectName(IDXGIObject *object, std::string_view name) {
    object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT) name.size(), name.data());
}

void PD::DXGIClearDebugObjectName(IDXGIObject *object) {
    object->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> PD::GetStaticSamplers() {
    // Applications usually only need a handful of samplers.  So just define them all up front
    // and keep them available as part of the root signature.

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(0,                               // shaderRegister
                                                D3D12_FILTER_MIN_MAG_MIP_POINT,  // filter
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP);// addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,                                // shaderRegister
                                                 D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);// addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(2,                               // shaderRegister
                                                 D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP);// addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,                                // shaderRegister
                                                  D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP);// addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(4,                              // shaderRegister
                                                      D3D12_FILTER_ANISOTROPIC,       // filter
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,// addressU
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,// addressV
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,// addressW
                                                      0.0f,                           // mipLODBias
                                                      8);                             // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,                               // shaderRegister
                                                       D3D12_FILTER_ANISOTROPIC,        // filter
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// addressU
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// addressV
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// addressW
                                                       0.0f,                            // mipLODBias
                                                       8);                              // maxAnisotropy

    return {pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp};
}

void PD::CreateDummy1x1Texture(Texture &inTex, ID3D12Device *device, ID3D12GraphicsCommandList *cmdList,
                               DirectX::XMFLOAT4 Color) {
    assert(inTex.Resource == nullptr);
    assert(inTex.UploadHeap == nullptr);
    auto ImageTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1, 1, 1);
    HR(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                       &ImageTextureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                       IID_PPV_ARGS(inTex.Resource.GetAddressOf())));


    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = &Color;
    textureData.RowPitch = static_cast<LONG_PTR>((1));
    textureData.SlicePitch = textureData.RowPitch * 1;
    subresources.push_back(textureData);

    UINT64 uploadBufferSize = GetRequiredIntermediateSize(inTex.Resource.Get(), 0, subresources.size());

    // Create the GPU upload buffer.
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

    auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                  IID_PPV_ARGS(inTex.UploadHeap.GetAddressOf())));
    UpdateSubresources(cmdList, inTex.Resource.Get(), inTex.UploadHeap.Get(), 0, 0,
                       static_cast<UINT>(subresources.size()), subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(inTex.Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}

void PD::Texture::LoadAndUploadTexture(Texture &texture, ID3D12Device *device, ID3D12GraphicsCommandList *cmdList,
                                       bool bFlip) {
    assert(!texture.Filename.empty());
    auto path = fs::path(texture.Filename);
    auto suffix = path.extension().u8string();

    std::unique_ptr<uint8_t[]> imageData;

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    if (suffix == ".DDS" || suffix == ".dds") {
        ThrowIfFailed(DirectX::LoadDDSTextureFromFile(
                device, path.wstring().c_str(), texture.Resource.ReleaseAndGetAddressOf(), imageData, subresources));
    } else {
        int ImageWidth;
        int ImageHeight;
        int ImageChannels;
        int ImageDesiredChannels = 4;

        stbi_set_flip_vertically_on_load(bFlip);
        imageData.reset(
                stbi_load(path.u8string().c_str(), &ImageWidth, &ImageHeight, &ImageChannels, ImageDesiredChannels));
        assert(imageData);
        stbi_set_flip_vertically_on_load(!bFlip);

        auto ImageTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, ImageWidth, ImageHeight, 1, 1);
        HR(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                           &ImageTextureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                           IID_PPV_ARGS(texture.Resource.GetAddressOf())));

        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = imageData.get();
        // https://github.com/microsoft/DirectXTex/wiki/ComputePitch
        // The rowPitch is the number of bytes in a scanline of pixels in the image. A standard pitch is 'byte' aligned and therefore it is equal to bytes-per-pixel * width-of-image.
        // textureData.RowPitch = static_cast<LONG_PTR>((ImageChannels * ImageWidth));
        textureData.RowPitch = static_cast<LONG_PTR>((4 * ImageWidth));
        textureData.SlicePitch = textureData.RowPitch * ImageHeight;
        subresources.push_back(textureData);
    }
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Resource.Get(), 0, subresources.size());

    // Create the GPU upload buffer.
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

    auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    ThrowIfFailed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                  IID_PPV_ARGS(texture.UploadHeap.GetAddressOf())));

    //https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-updatesubresource

    UpdateSubresources(cmdList, texture.Resource.Get(), texture.UploadHeap.Get(), 0, 0,
                       static_cast<UINT>(subresources.size()), subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);
}
