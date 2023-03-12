#pragma once

#include "d3dUtils.hh"


namespace PD{

template <typename T>
class UploadBuffer : Noncopyable
{
    public:
    UploadBuffer(ID3D12Device* device,uint32_t elementCount,bool bIsConstantBuffer)
    :mbIsConstantBuffer(bIsConstantBuffer)
    {
        assert(device);
        mElementByteSize = bIsConstantBuffer ? CalcConstantBufferBytesSize(sizeof(T)) : sizeof(T);

        auto HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto HeapDesc = CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount);
        HR(device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &HeapDesc,
                                           D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                           IID_PPV_ARGS(mUploadBuffer.GetAddressOf())));

        // TODO: 阅读这个函数的文档
        HR(mUploadBuffer->Map(0, nullptr, (void **) &mMappedData));
    }
    ~UploadBuffer()
    {
        if(mUploadBuffer)
        {
            mUploadBuffer->Unmap(0, nullptr);
        }
        mMappedData = (uint8_t *)(uintptr_t)0xDEADDEAD;
    }

    ID3D12Resource* Resource() const
    {
        return mUploadBuffer.Get();
    }

    // 一次只能上传一个元素
    void CopyData(int elementIndex,const T& Data)
    {
        assert(mMappedData);
        memmove(mMappedData + elementIndex * mElementByteSize, (const char*)&Data , sizeof(T));
    }

    uint32_t getElementBytesSize() const { return mElementByteSize; }

    uint8_t *getGPUVirtualAddress() const {
        assert(mMappedData);
        return mMappedData;
    }


private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    uint8_t* mMappedData = nullptr;
    uint32_t mElementByteSize = 0;
    bool mbIsConstantBuffer = false;
};
}
