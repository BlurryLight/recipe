#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "UploadBuffer.hh"
#include "Waves.h"
#include "resource_path_searcher.h"
#include <array>
#include <d3dApp.hh>
#include <spdlog/spdlog.h>


#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"


using namespace PD;

using namespace DirectX;

using DirectX::PackedVector::XMCOLOR;
const static int kNumFrameResources = 3;

// RenderItem is related to per object
struct RenderItem {
    XMFLOAT4X4 World = MathHelper::Identity4x4();
    XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
    // init: every resource needs to be updated
    int NumFramesDirty = kNumFrameResources;

    UINT ObjectCBIndex = -1;
    Material *Mat = nullptr;
    MeshGeometry *Geo = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // draw instance params
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};


class LandAndWavesBlendApp : public D3DApp {
public:
    LandAndWavesBlendApp(HINSTANCE hInstance);
    virtual void ReleaseResource() override;
    virtual bool Initialize() override;
    void Update(const GameTimer &timer) override;
    void Draw(const GameTimer &gt) override;

private:
    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildMaterials();
    void LoadTextures();
    void UpdateMaterialCBs(const GameTimer &timer);
    void AnimateMaterials(const GameTimer &timer);

    void BuildRootSignature();
    void BuildShaderAndInputLayout();
    void BuildLandGeometry();
    void BuildWavesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRenderItems();
    void UpdateWaves(const GameTimer &gt);

    void UpdateObjectCB(const GameTimer &timer);
    void UpdateMainPassCB(const GameTimer &timer);
    void DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<const RenderItem *> &ritems);
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ResourcePathSearcher mResourceManager;

    std::unordered_map<std::string, ComPtr<ID3D10Blob>> mShaders;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, std::unique_ptr<PD::Texture>> mTextures;
    std::unordered_map<std::string, int> mTextureSrvHandleIndices;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    using RenderItemLayer = std::vector<const RenderItem *>;
    RenderItemLayer mOpaqueRitems;
    RenderItemLayer mTransparentRitems;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();
    UINT mPassCbvOffset = 0;
    int mCurrFrameResourceIndex = 0;

    FrameResource *mCurrFrameResource = nullptr;
    PassConstants mMainPassCB;
    bool mbShowWireFrame = false;
    bool mbVsync = true;
    std::unique_ptr<Waves> mWaves = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
};

inline LandAndWavesBlendApp::LandAndWavesBlendApp(HINSTANCE hInstance) : D3DApp(hInstance) {
    auto path = PD::ResourcePathSearcher::Path(PROJECT_DIR);
    if (!path.is_absolute()) { path = fs::absolute(path); }
    mResourceManager.add_path(path);
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "models");
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "textures");
    mResourceManager.add_path(path / L"Shaders");
}

inline void LandAndWavesBlendApp::ReleaseResource() { mRootSignature.Reset(); }

inline bool LandAndWavesBlendApp::Initialize() {
    if (!D3DApp::Initialize()) { return false; }
    mCamera = new PD::Camera(SimpleMath::Vector3(0, 1, -5), SimpleMath::Vector3(0, 0, 1), SimpleMath::Vector3(0, 1, 0));
    mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
    assert(mCommandList);
    assert(mDirectCmdListAlloc);
    HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    // ready for record
    LoadTextures();
    BuildRootSignature();
    BuildShaderAndInputLayout();
    BuildLandGeometry();
    BuildWavesGeometry();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildPSOs();

    // end of record
    HR(mCommandList->Close());
    std::array<ID3D12CommandList *, 1> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
    FlushCommandQueue();

    return true;
}

inline void LandAndWavesBlendApp::Update(const GameTimer &timer) {

    D3DApp::Update(timer);
    XMStoreFloat4x4(&mView, mCamera->GetViewMatrix());

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    float aspect = GetAspectRatio();
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspect, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % kNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // 此时GPU已经落后CPU kNumFrameResources帧
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        HR(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
    UpdateWaves(timer);
    UpdateObjectCB(timer);
    UpdateMainPassCB(timer);
    AnimateMaterials(timer);
    UpdateMaterialCBs(timer);

    ImGuiPrepareDraw();
    static bool bShowDemoWindow = false;
    if (bShowDemoWindow) ImGui::ShowDemoWindow(&bShowDemoWindow);

    ImGui::Begin("LandScape!");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::SliderFloat("MoveSpeed", &mCamera->MovementSpeed, 0.0f, 50.0f);
    ImGui::Checkbox("WireFrame", &mbShowWireFrame);
    ImGui::Checkbox("VSync", &mbVsync);
    ImGui::Checkbox("DemoWindow", &bShowDemoWindow);
    ImGui::End();
    ImGui::Render();
}

void LandAndWavesBlendApp::UpdateObjectCB(const GameTimer &timer) {
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto &element : mAllRitems) {
        // upload data when needed
        if (element->NumFramesDirty > 0) {
            XMMATRIX world = XMLoadFloat4x4(&element->World);
            ObjectConstants objCB;
            XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objCB.InvTransWorld, XMMatrixTranspose(MathHelper::InverseTranspose(world)));
            XMStoreFloat4x4(&objCB.TexTransform, XMLoadFloat4x4(&element->TexTransform));
            currObjectCB->CopyData(element->ObjectCBIndex, objCB);
            element->NumFramesDirty--;
        }
    }
}
inline void LandAndWavesBlendApp::UpdateMaterialCBs(const GameTimer &timer) {

    auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
    for (auto &element : mMaterials) {
        // upload data when needed
        Material *mat = element.second.get();
        if (mat->NumFramesDirty > 0) {
            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            auto matTrans = XMLoadFloat4x4(&mat->MatTransform);
            XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTrans));
            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);
            mat->NumFramesDirty--;
        }
    }
}
inline void LandAndWavesBlendApp::AnimateMaterials(const GameTimer &timer) {
    auto mat = mMaterials.at("water").get();
    float &tu = mat->MatTransform(3, 0);
    float &tv = mat->MatTransform(3, 1);

    tu += 0.1f * timer.DeltaTime();
    tv += 0.02f * timer.DeltaTime();
    if (tu >= 1.0f) tu = -1.0f;
    if (tv >= 1.0f) tv = -1.0f;// 需要使用wrap采样

    mat->NumFramesDirty = kNumFrameResources;
}
void LandAndWavesBlendApp::UpdateMainPassCB(const GameTimer &gt) {
    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB.EyePosW = mCamera->Position;
    mMainPassCB.RenderTargetSize = XMFLOAT2((float) mWidth, (float) mHeight);
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mWidth, 1.0f / mHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();
    mMainPassCB.AmbientLight = {0.25f, 0.25f, 0.35f, 1.0f};

    mMainPassCB.Lights[0].Direction = {0.57735f, -0.57735f, 0.57735f};
    mMainPassCB.Lights[0].Strength = {0.9f, 0.9f, 0.8f};
    mMainPassCB.Lights[1].Direction = {-0.57735f, -0.57735f, 0.57735f};
    mMainPassCB.Lights[1].Strength = {0.3f, 0.3f, 0.3f};
    mMainPassCB.Lights[2].Direction = {0.0f, -0.707f, -0.707f};
    mMainPassCB.Lights[2].Strength = {0.15f, 0.15f, 0.15f};

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

inline void LandAndWavesBlendApp::DrawRenderItems(ID3D12GraphicsCommandList *cmdList,
                                                  const std::vector<const RenderItem *> &ritems) {

    uint32_t objCBByteSize = CalcConstantBufferBytesSize(sizeof(ObjectConstants));
    uint32_t matCBByteSize = CalcConstantBufferBytesSize(sizeof(MaterialConstants));
    auto objectCB = mCurrFrameResource->ObjectCB->Resource();
    auto matCB = mCurrFrameResource->MaterialCB->Resource();
    for (size_t i = 0; i < ritems.size(); i++) {
        // 要绘制一个物体，需要物体本身的信息，和它有关的model
        // 先设置顶点信息
        auto ri = ritems[i];
        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS ObjCBStartAddress = objectCB->GetGPUVirtualAddress();
        D3D12_GPU_VIRTUAL_ADDRESS MatCBStartAddress = matCB->GetGPUVirtualAddress();
        auto ObjCBAddress = ObjCBStartAddress + ri->ObjectCBIndex * objCBByteSize;
        auto MatCBAddress = MatCBStartAddress + ri->Mat->MatCBIndex * matCBByteSize;

        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);
        cmdList->SetGraphicsRootConstantBufferView(0, ObjCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(1, MatCBAddress);
        cmdList->SetGraphicsRootDescriptorTable(3, tex);
        assert(ri->IndexCount > 0);
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

void LandAndWavesBlendApp::Draw(const GameTimer &gt) {


    // 取出当前帧的Alloc,然后把cmdlist关联到这个alloc
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    HR(cmdListAlloc->Reset());

    auto SwitchPSO = [this]() { return mbShowWireFrame ? mPSOs["opaque_wireframe"].Get() : mPSOs["opaque"].Get(); };
    HR(mCommandList->Reset(cmdListAlloc.Get(), SwitchPSO()));

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);


    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float *) &mMainPassCB.FogColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0,
                                        0, nullptr);


    auto passCBAddr = mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress();
    std::vector<ID3D12DescriptorHeap *> descriptorHeaps{mSrvDescriptorHeap.Get()};
    mCommandList->SetDescriptorHeaps(descriptorHeaps.size(), descriptorHeaps.data());
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddr);

    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

    mCommandList->SetPipelineState(mPSOs.at("transparent_blend").Get());
    DrawRenderItems(mCommandList.Get(), mTransparentRitems);

    mCommandList->SetPipelineState(mPSOs["opaque"].Get());
    mCommandList->SetDescriptorHeaps(1, mImGuiCbvHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());


    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                           D3D12_RESOURCE_STATE_PRESENT));
    HR(mCommandList->Close());
    std::vector<ID3D12CommandList *> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());

    // turn vsync on
    HR(mSwapChain->Present(mbVsync, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % kSwapChainBufferCount;

    // CPU往FrameResource里记录一下新的Fence点，当这一帧完成的时候，Fence应该为 ++mCurrentFence
    // 同时往GPU插一条命令，当GPU任务执行完时，应该把mFence的值设置为mCurrentFence
    mCurrFrameResource->Fence = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

inline void LandAndWavesBlendApp::BuildDescriptorHeaps() {
    assert(mRtvHeap.Get());
    assert(mDsvHeap.Get());
    spdlog::info("Building Descriptor Heaps");

    UINT ObjCount = (UINT) mOpaqueRitems.size();
    // ObjCount 是per obj的cb
    // +1 是整个Pass共通的cb
    UINT numDescriptor = (ObjCount + 1) * kNumFrameResources;

    mPassCbvOffset = ObjCount * kNumFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = numDescriptor;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));

    // srv

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.NumDescriptors = mTextures.size();
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));


    for (const auto &it : mTextures) {
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        srvHandle.Offset(mTextureSrvHandleIndices.at(it.first), mCbvSrvUavDescriptorSize);

        auto resource = it.second->Resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        mD3dDevice->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
    }
}

inline void LandAndWavesBlendApp::BuildConstantBuffers() {
    spdlog::info("Building Constant Buffers");

    spdlog::info("Building Objects Constant Buffers");
    uint32_t ObjCBBytesSize = CalcConstantBufferBytesSize(sizeof(ObjectConstants));
    UINT ObjCount = (UINT) mOpaqueRitems.size();
    for (int frameIndex = 0; frameIndex < kNumFrameResources; ++frameIndex) {
        auto ObjectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
        for (uint32_t i = 0; i < ObjCount; i++) {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = ObjectCB->GetGPUVirtualAddress();
            // 第几个物体的ConstBuffer
            cbAddress += i * ObjCBBytesSize;
            int heapIndex = frameIndex * ObjCount + i;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbAddress;
            cbvDesc.SizeInBytes = ObjCBBytesSize;
            mD3dDevice->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    spdlog::info("Building Passes Constant Buffers");
    UINT passCBByteSize = CalcConstantBufferBytesSize(sizeof(PassConstants));

    // Last three descriptors are the pass CBVs for each frame resource.
    for (int frameIndex = 0; frameIndex < kNumFrameResources; ++frameIndex) {
        auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

        // Offset to the pass cbv in the descriptor heap.
        assert(mPassCbvOffset > 0);
        int heapIndex = mPassCbvOffset + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = passCBByteSize;
        mD3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }
}
inline void LandAndWavesBlendApp::BuildRootSignature() {

    CD3DX12_ROOT_PARAMETER slotRootParameter[4];
    // 一个放Object，一个放PassCB,一个放MatCB
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);

    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    auto staticSamplers = PD::GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter, staticSamplers.size(),
                                            staticSamplers.data(),
                                            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;// 如果创建root sig失败了，里面会存放错误信息

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
    if (errorBlob) { spdlog::warn("Root Sig error: {}", (const char *) (errorBlob->GetBufferPointer())); }
    HR(hr);

    HR(mD3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
                                       IID_PPV_ARGS(&mRootSignature)));
}

inline void LandAndWavesBlendApp::BuildShaderAndInputLayout() {
    spdlog::info("Bulding Shaders");
    auto ShaderPath = mResourceManager.find_path("color.hlsl");

    auto VSBlob = CompileShader(ShaderPath, nullptr, "VSMain", "vs_5_0");
    const D3D_SHADER_MACRO FogDefines[] = {{"FOG", "1"}, {NULL, NULL}};
    auto PSBlob = CompileShader(ShaderPath, FogDefines, "PSMain", "ps_5_0");
    assert(VSBlob);
    assert(PSBlob);
    mShaders["standardVS"] = VSBlob;
    mShaders["opaquePS"] = PSBlob;

    spdlog::info("Bulding Input Layout");
    mInputLayout = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
             0},
            {
                    "TEXCOORD",
                    0,
                    DXGI_FORMAT_R32G32_FLOAT,
                    0,
                    sizeof(XMFLOAT3) + sizeof(XMFLOAT3),
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    0,
            },
    };
}

inline void LandAndWavesBlendApp::BuildLandGeometry() {
    spdlog::info("Bulding LandScape Geometries");

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

    std::vector<Vertex> vertices(grid.Vertices.size());

    // y = f(x,z)
    auto HeightFunc = [](float x, float z) -> float { return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z)); };
    auto NormalFunc = [](float x, float z) {
        // n = (-df/dx, 1, -df/dz)
        XMFLOAT3 n(-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z), 1.0f,
                   -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

        XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
        XMStoreFloat3(&n, unitNormal);

        return n;
    };
    size_t k = 0;
    for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k) {
        auto &p = grid.Vertices[k].Position;
        vertices[k].Pos = p;
        vertices[k].Pos.y = HeightFunc(p.x, p.z);
        vertices[k].Normal = NormalFunc(p.x, p.z);
        vertices[k].TexC = grid.Vertices[i].TexC;
    }
    std::vector<uint16_t> indices;
    indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());

    const UINT vbByteSize = (UINT) vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT) indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "landGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize,
                                               geo->VertexBufferUploader);

    geo->IndexBufferGPU =
            CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), geo->IndexBufferCPU->GetBufferPointer(),
                                ibByteSize, geo->IndexBufferUploader);

    geo->VertexBytesStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferBytesSize = ibByteSize;

    SubmeshGeometry gridSubmesh;
    gridSubmesh.BaseVertexLocation = 0;
    gridSubmesh.StartIndexLocation = 0;
    gridSubmesh.IndexCount = grid.Indices32.size();

    geo->DrawArgs["land"] = gridSubmesh;

    mGeometries[geo->name] = std::move(geo);
}

inline void LandAndWavesBlendApp::BuildWavesGeometry() {

    spdlog::info("Bulding Waves Geometries");

    std::vector<uint16_t> indices(3 * mWaves->TriangleCount());
    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for (int i = 0; i < m - 1; ++i) {
        for (int j = 0; j < n - 1; ++j) {

            /*
                0 ---- 1 (4)
                |   /   |
                | /     |
                2 (3)----5
            */
            indices[k] = i * n + j;
            indices[k + 1] = i * n + j + 1;
            indices[k + 2] = (i + 1) * n + j;

            indices[k + 3] = (i + 1) * n + j;
            indices[k + 4] = i * n + j + 1;
            indices[k + 5] = (i + 1) * n + j + 1;

            k += 6;// next quad
        }
    }

    const UINT vbByteSize = (UINT) mWaves->VertexCount() * sizeof(Vertex);
    const UINT ibByteSize = (UINT) indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "waveGeo";


    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->IndexBufferGPU =
            CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), geo->IndexBufferCPU->GetBufferPointer(),
                                ibByteSize, geo->IndexBufferUploader);

    geo->VertexBytesStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferBytesSize = ibByteSize;

    SubmeshGeometry waveSubmesh;
    waveSubmesh.BaseVertexLocation = 0;
    waveSubmesh.StartIndexLocation = 0;
    waveSubmesh.IndexCount = indices.size();
    geo->DrawArgs["wave"] = waveSubmesh;

    mGeometries[geo->name] = std::move(geo);
}

inline void LandAndWavesBlendApp::BuildPSOs() {

    spdlog::info("Building PSO");
    spdlog::info("Building Opaque PSO");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{0};
    psoDesc.InputLayout = {mInputLayout.data(), (UINT) mInputLayout.size()};
    psoDesc.pRootSignature = mRootSignature.Get();

    psoDesc.VS = {(void *) (mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize()};
    psoDesc.PS = {(void *) (mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize()};

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // PSO这里指定的 Primitive 类型 和 IA那里指定的 TriangleStrip，需要是同一大类，但是不需完全匹配上
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.DSVFormat = mDepthStencilFormat;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    {
        spdlog::info("Building WireFrame PSO");
        auto tmp = psoDesc;
        tmp.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
    }

    {
        spdlog::info("Building blend PSO");
        // rgb  c_src * a_src + dst * (1 - a_src)
        // a   a_src * 1 + a_dst * 0
        auto tmp = psoDesc;
        D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
        transparencyBlendDesc.BlendEnable = true;
        transparencyBlendDesc.LogicOpEnable = false;
        transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
        transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
        transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        tmp.BlendState.RenderTarget[0] = transparencyBlendDesc;
        tmp.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mPSOs["transparent_blend"])));
    }
}

inline void LandAndWavesBlendApp::BuildFrameResources() {
    for (int i = 0; i < kNumFrameResources; i++) {
        mFrameResources.push_back(std::make_unique<FrameResource>(mD3dDevice.Get(), 1, (UINT) mAllRitems.size(),
                                                                  mMaterials.size(), mWaves->VertexCount()));
    }
}

inline void LandAndWavesBlendApp::BuildRenderItems() {
    int objectIndex = 0;
    spdlog::info("Build Render Items");

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
    gridRitem->ObjectCBIndex = objectIndex++;
    gridRitem->Geo = mGeometries["landGeo"].get();
    gridRitem->Mat = mMaterials.at("grass").get();
    gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs.at("land").IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs.at("land").StartIndexLocation;
    XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));

    auto waveRitem = std::make_unique<RenderItem>();
    waveRitem->World = MathHelper::Identity4x4();
    XMStoreFloat4x4(&waveRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
    waveRitem->ObjectCBIndex = objectIndex++;
    waveRitem->Geo = mGeometries["waveGeo"].get();
    waveRitem->Mat = mMaterials.at("water").get();
    waveRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    waveRitem->IndexCount = waveRitem->Geo->DrawArgs["wave"].IndexCount;
    waveRitem->StartIndexLocation = waveRitem->Geo->DrawArgs["wave"].StartIndexLocation;
    waveRitem->BaseVertexLocation = waveRitem->Geo->DrawArgs["wave"].BaseVertexLocation;

    mOpaqueRitems.push_back(gridRitem.get());
    mTransparentRitems.push_back(waveRitem.get());

    mAllRitems.push_back(std::move(gridRitem));
    mAllRitems.push_back(std::move(waveRitem));
}

inline void LandAndWavesBlendApp::LoadTextures() {
    int texIndex = 0;
    {
        auto dummyTex = std::make_unique<Texture>();
        dummyTex->Name = "dummy";
        dummyTex->Filename = dummyTex->Name;
        CreateDummy1x1Texture(*dummyTex, mD3dDevice.Get(), mCommandList.Get(), XMFLOAT4(1, 1, 1, 1));
        mTextureSrvHandleIndices[dummyTex->Name] = texIndex++;
        mTextures[dummyTex->Name] = std::move(dummyTex);
    }
    {
        auto waterTex = std::make_unique<Texture>();
        waterTex->Name = "water";
        waterTex->Filename = fs::absolute(this->mResourceManager.find_path("water1.dds")).u8string();
        mTextureSrvHandleIndices[waterTex->Name] = texIndex++;
        Texture::LoadAndUploadTexture(*waterTex, mD3dDevice.Get(), mCommandList.Get());
        mTextures[waterTex->Name] = std::move(waterTex);
    }

    {
        auto grassTex = std::make_unique<Texture>();
        grassTex->Name = "grass";
        grassTex->Filename = fs::absolute(this->mResourceManager.find_path("grass.dds")).u8string();
        Texture::LoadAndUploadTexture(*grassTex, mD3dDevice.Get(), mCommandList.Get());
        mTextureSrvHandleIndices[grassTex->Name] = texIndex++;
        mTextures[grassTex->Name] = std::move(grassTex);
    }
}

inline void LandAndWavesBlendApp::BuildMaterials() {
    int matIndex = 0;
    {
        auto DummyWhite = std::make_unique<Material>();
        DummyWhite->Name = "dummywhite";
        DummyWhite->MatCBIndex = matIndex++;
        DummyWhite->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("dummy");
        mMaterials["dummywhite"] = std::move(DummyWhite);
    }

    {
        auto water = std::make_unique<Material>();
        water->Name = "water";
        water->MatCBIndex = matIndex++;
        water->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("water");
        water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.6f);
        water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
        water->Roughness = 0.f;
        mMaterials["water"] = std::move(water);
    }

    {
        auto grass = std::make_unique<Material>();
        grass->Name = "grass";
        grass->MatCBIndex = matIndex++;
        grass->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("grass");
        grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
        grass->Roughness = 0.125f;

        mMaterials["grass"] = std::move(grass);
    }
}

// 这个方法属实很蠢
// 每一帧的时候更新所有的顶点位置。
// 正式场景不要用
inline void LandAndWavesBlendApp::UpdateWaves(const GameTimer &gt) {
    static float t_base = 0.0f;
    if ((mTimer.TotalTime() - t_base) >= 0.25f) {
        t_base += 0.25f;

        int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
        int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

        float r = MathHelper::RandF(0.2f, 0.5f);

        mWaves->Disturb(i, j, r);
    }
    auto WaveGeo = mGeometries.at("waveGeo").get();
    auto currWavesVB = mCurrFrameResource->WavesUploader.get();

    mWaves->Update(gt.DeltaTime());

    // Update the wave vertex buffer with the new solution.
    for (int i = 0; i < mWaves->VertexCount(); ++i) {
        Vertex v;

        v.Pos = mWaves->Position(i);
        v.Normal = mWaves->Normal(i);

        // // Derive tex-coords from position by
        // // mapping [-w/2,w/2] --> [0,1]
        v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
        v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

        currWavesVB->CopyData(i, v);
    }

    // Set the dynamic VB of the wave renderitem to the current frame VB.
    WaveGeo->VertexBufferGPU = currWavesVB->Resource();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    LandAndWavesBlendApp theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}