#pragma once

#include <array>
#include <d3dUtils.hh>


namespace PD {

    enum class CubeMapFace : int32_t {
        PositiveX = 0,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ,
    };

    class CubeRenderTarget : public Noncopyable {
    public:
        CubeRenderTarget(ID3D12Device *device, uint32_t width, uint32_t height,
                         DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);
        ID3D12Resource *Resource();

        D3D12_VIEWPORT GetViewport() const { return mViewport; }
        D3D12_RECT GetScissorRect() const { return mScissorRect; }
        CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrv() const { return mhGpuSrv; }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int faceIndex) const { return mhCpuRtv.at(faceIndex); }

        void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
                              CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]);

        void OnResize(UINT newWidth, UINT newHeight);

    private:
        void BuildInternalDescroptors();
        void BuildResource();

    private:
        ID3D12Device *mDevice = nullptr;
        uint32_t mWidth = 0;
        uint32_t mHeight = 0;
        DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;
        CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
        CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
        std::array<CD3DX12_CPU_DESCRIPTOR_HANDLE, 6> mhCpuRtv;
        ComPtr<ID3D12Resource> mCubeMap = nullptr;
    };
};// namespace PD