#include "FrameResource.h"
#include "UploadBuffer.hh"
using namespace PD;
PD::FrameResource::FrameResource(ID3D12Device *device, UINT passCount, UINT maxObjectInstance, UINT materialCount) {
    HR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    ObjectInstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, maxObjectInstance, false);
    MaterialStructuralBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
}
