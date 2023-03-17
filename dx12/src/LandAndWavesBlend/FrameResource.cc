#include "FrameResource.h"
#include "UploadBuffer.hh"
using namespace PD;
PD::FrameResource::FrameResource(ID3D12Device *device, UINT passCount, UINT objectCount, UINT materialCount,
                                 UINT waveVertices) {
    HR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
    MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
    WavesUploader = std::make_unique<UploadBuffer<Vertex>>(device, waveVertices, false);
}
