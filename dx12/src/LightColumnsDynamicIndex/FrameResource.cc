#include "FrameResource.h"
#include "UploadBuffer.hh"
using namespace PD;
PD::FrameResource::FrameResource(ID3D12Device *device, UINT passCount, UINT objectCount, UINT materialCount) {
    HR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
    MaterialStructuralBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
}
