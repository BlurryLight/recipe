// gaussian blur in compute shader
#pragma once
#include <array>
#include <d3dUtils.hh>


namespace PD {
    class BlurFilter : public Noncopyable {
    public:
        void OnResize(uint32_t newWidth, uint32_t newHeight);
        void Execute(ID3D12GraphicsCommandList *cmdList, ID3D12RootSignature *rootSig, ID3D12PipelineState *horzBlurPSO,
                     ID3D12PipelineState *vertBlurPSO, ID3D12Resource *input, int blurCount);
        ID3D12Resource *Output() { return mBlurMap0.Get(); }

        void BuildResources();
        void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
                              CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor, uint32_t descriptorSize);
        BlurFilter(ID3D12Device *device, uint32_t width, uint32_t height,
                   DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

    private:
        void BuildInternalDescriptors();
        std::vector<float> CalcGaussWeights(float sigma);
        static constexpr int kMaxBlurRadius = 5;// 5px
        ID3D12Device *mDevice = nullptr;
        uint32_t mWidth = 0;
        uint32_t mHeight = 0;
        DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuSrv;
        CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuUav;

        CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuSrv;
        CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuUav;

        CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuSrv;
        CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuUav;

        CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuSrv;
        CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuUav;

        // Two for ping-ponging the textures.
        Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap0 = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap1 = nullptr;
    };
}// namespace PD