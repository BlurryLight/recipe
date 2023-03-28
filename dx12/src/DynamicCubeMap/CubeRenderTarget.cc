#include "CubeRenderTarget.h"

using namespace PD;
PD::CubeRenderTarget::CubeRenderTarget(ID3D12Device *device, uint32_t width, uint32_t height, DXGI_FORMAT format)
    : mDevice(device), mWidth(width), mHeight(height), mFormat(format) {
    mViewport = {0, 0, (float) width, (float) height, 0.f, 1.f};
    mScissorRect = {0, 0, (int) width, (int) height};
    BuildResource();
}
ID3D12Resource *PD::CubeRenderTarget::Resource() {
    assert(mCubeMap);
    return mCubeMap.Get();
}

void PD::CubeRenderTarget::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
                                            CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
                                            CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]) {
    mhCpuSrv = hCpuSrv;
    mhGpuSrv = hGpuSrv;
    for (int i = 0; i < 6; i++) { mhCpuRtv[i] = hCpuRtv[i]; }
    BuildInternalDescroptors();
}

void PD::CubeRenderTarget::OnResize(UINT newWidth, UINT newHeight) {
    if (mWidth != newWidth || mHeight != newHeight) {
        mWidth = newWidth;
        mHeight = newHeight;
        BuildResource();
        BuildInternalDescroptors();
    }
}

void PD::CubeRenderTarget::BuildInternalDescroptors() {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0;
    mDevice->CreateShaderResourceView(mCubeMap.Get(), &srvDesc, mhCpuSrv);

    for (int i = 0; i < 6; i++) {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Format = mFormat;
        rtvDesc.Texture2DArray.MipSlice = 0;
        rtvDesc.Texture2DArray.PlaneSlice = 0;

        // Render target to ith element.
        rtvDesc.Texture2DArray.FirstArraySlice = i;

        // Only view one element of the array.
        // starting from FirstArraySlice
        rtvDesc.Texture2DArray.ArraySize = 1;

        // Create RTV to ith cubemap face.
        mDevice->CreateRenderTargetView(mCubeMap.Get(), &rtvDesc, mhCpuRtv[i]);
    }
}

void PD::CubeRenderTarget::BuildResource() {
    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(mFormat, mWidth, mHeight, 6, 1);
    texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    CD3DX12_CLEAR_VALUE cubeMapClearValue;
    cubeMapClearValue.Format = mFormat;
    memmove(&cubeMapClearValue.Color, DirectX::Colors::Red, sizeof(float) * 4);

    HR(mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
                                        &texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, &cubeMapClearValue,
                                        IID_PPV_ARGS(mCubeMap.GetAddressOf())));
}
