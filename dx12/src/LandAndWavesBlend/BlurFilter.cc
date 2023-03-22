#include "BlurFilter.h"
using namespace PD;
PD::BlurFilter::BlurFilter(ID3D12Device *device, uint32_t width, uint32_t height, DXGI_FORMAT format)
    : mDevice(device), mWidth(width), mHeight(height), mFormat(format) {
    BuildResources();
}

void PD::BlurFilter::OnResize(uint32_t newWidth, uint32_t newHeight) {
    if ((mWidth != newWidth) || (mHeight != newHeight)) {
        mWidth = newWidth;
        mHeight = newHeight;

        BuildResources();
        // 因为resource重建了，所以handle也需要更新
        BuildInternalDescriptors();
    }
}

void PD::BlurFilter::Execute(ID3D12GraphicsCommandList *cmdList, ID3D12RootSignature *rootSig,
                             ID3D12PipelineState *horzBlurPSO, ID3D12PipelineState *vertBlurPSO, ID3D12Resource *input,
                             int blurCount) {
    static auto weights = CalcGaussWeights(2.5);// 11x11 kernel,radius = 5
    assert(weights.size() % 2 == 1);
    static int blurRadius = (weights.size() - 1) / 2;

    cmdList->SetComputeRootSignature(rootSig);
    cmdList->SetComputeRoot32BitConstant(0, blurRadius, 0);
    cmdList->SetComputeRoot32BitConstants(0, weights.size(), weights.data(), 1);

    // blit backbuffer to mblur0
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                      D3D12_RESOURCE_STATE_COPY_SOURCE));
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                                      D3D12_RESOURCE_STATE_COPY_DEST));
    cmdList->CopyResource(mBlurMap0.Get(), input);

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                      D3D12_RESOURCE_STATE_GENERIC_READ));
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    for (int i = 0; i < blurCount; i++) {
        cmdList->SetPipelineState(horzBlurPSO);
        cmdList->SetComputeRootDescriptorTable(1, mBlur0GpuSrv);
        cmdList->SetComputeRootDescriptorTable(2, mBlur1GpuUav);

        auto numGroupsX = (uint32_t) ceil(mWidth / 256.0f);
        cmdList->Dispatch(numGroupsX, mHeight, 1);

        //ping-pong
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(),
                                                                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                          D3D12_RESOURCE_STATE_GENERIC_READ));
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
                                                                          D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        cmdList->SetPipelineState(vertBlurPSO);
        cmdList->SetComputeRootDescriptorTable(1, mBlur1GpuSrv);
        cmdList->SetComputeRootDescriptorTable(2, mBlur0GpuUav);

        auto numGroupsY = (uint32_t) ceil(mHeight / 256.0f);
        cmdList->Dispatch(mWidth, numGroupsY, 1);

        //放到最后是确保最后一次blur执行完后在正确的状态
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
                                                                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                                                          D3D12_RESOURCE_STATE_GENERIC_READ));
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(),
                                                                          D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                          D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    }
}

std::vector<float> PD::BlurFilter::CalcGaussWeights(float sigma) {
    assert(sigma > 0);
    // 龙书里直接假设radius为 2.0 * sigma
    float inv_2sigma2 = 1.0f / (2.0f * sigma * sigma);
    int radius = (int) ceil(sigma * 2.0);
    radius = std::min(radius, kMaxBlurRadius);
    std::vector<float> weights(2 * radius + 1, 0);// radius = 3 7x7 kernal
    float weightSum = 0.0f;
    for (int i = -radius; i <= radius; i++) {
        float tmp = -(float) i * float(i);
        weights[i + radius] = expf(tmp * inv_2sigma2);
        weightSum += weights[i + radius];
    }
    for (auto &weight : weights) { weight /= weightSum; }
    return weights;
}

void PD::BlurFilter::BuildResources() {
    auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(mFormat, mWidth, mHeight, 1, 1);
    texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    HR(mDevice->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COMMON,
                                        nullptr, IID_PPV_ARGS(mBlurMap0.GetAddressOf())));

    HR(mDevice->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_COMMON,
                                        nullptr, IID_PPV_ARGS(mBlurMap1.GetAddressOf())));
    D3D12SetDebugObjectName(mBlurMap0.Get(), "Blur0Map");
    D3D12SetDebugObjectName(mBlurMap1.Get(), "Blur1Map");
}
void PD::BlurFilter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
                                      CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor, uint32_t descriptorSize) {
    mBlur0CpuSrv = hCpuDescriptor;
    mBlur0CpuUav = hCpuDescriptor.Offset(1, descriptorSize);
    mBlur1CpuSrv = hCpuDescriptor.Offset(1, descriptorSize);
    mBlur1CpuUav = hCpuDescriptor.Offset(1, descriptorSize);

    mBlur0GpuSrv = hGpuDescriptor;
    mBlur0GpuUav = hGpuDescriptor.Offset(1, descriptorSize);
    mBlur1GpuSrv = hGpuDescriptor.Offset(1, descriptorSize);
    mBlur1GpuUav = hGpuDescriptor.Offset(1, descriptorSize);
    BuildInternalDescriptors();
}
void PD::BlurFilter::BuildInternalDescriptors() {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = mFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = mFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    assert(mBlur0CpuUav.ptr);
    assert(mBlur0CpuSrv.ptr);
    assert(mBlur1CpuUav.ptr);
    assert(mBlur1CpuSrv.ptr);

    mDevice->CreateShaderResourceView(mBlurMap0.Get(), &srvDesc, mBlur0CpuSrv);
    mDevice->CreateShaderResourceView(mBlurMap1.Get(), &srvDesc, mBlur1CpuSrv);

    mDevice->CreateUnorderedAccessView(mBlurMap0.Get(), nullptr, &uavDesc, mBlur0CpuUav);
    mDevice->CreateUnorderedAccessView(mBlurMap1.Get(), nullptr, &uavDesc, mBlur1CpuUav);
}
