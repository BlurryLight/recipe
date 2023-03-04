
#include <MathHelper.h>
#include <UploadBuffer.hh>
#include "d3dUtils.hh"

namespace PD {
    using namespace DirectX;
    struct ObjectConstants {
        DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
        DirectX::XMFLOAT4X4 InvTransWorld = MathHelper::Identity4x4();
        DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
    };


    struct PassConstants {
        XMFLOAT4X4 View = MathHelper::Identity4x4();
        XMFLOAT4X4 InvView = MathHelper::Identity4x4();
        XMFLOAT4X4 Proj = MathHelper::Identity4x4();
        XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
        XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
        XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
        XMFLOAT3 EyePosW{0, 0, 0};
        float cbPerObjectPad1;// padding for 4 bytes

        XMFLOAT2 RenderTargetSize{0, 0};
        XMFLOAT2 InvRenderTargetSize{0, 0};
        float NearZ = 0;
        float FarZ = 0;
        float TotalTime = 0;
        float DeltaTime = 0;
    };

    struct Vertex {
        XMFLOAT3 Pos;
        XMFLOAT3 Normal;
        XMFLOAT2 TexC;
    };

    struct FrameResource : Noncopyable {
        FrameResource(ID3D12Device *device, UINT passCount, UINT objectCount);
        ~FrameResource(){};
        // every frame needs it allocator
        ComPtr<ID3D12CommandAllocator> CmdListAlloc;
        std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
        std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
        UINT64 Fence = 0;
    };
}// namespace PD