#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "UploadBuffer.hh"
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
    // init: every resource needs to be updated
    int NumFramesDirty = kNumFrameResources;

    UINT ObjectCBIndex = -1;
    MeshGeometry *Geo = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    Material* Mat = nullptr;
    // draw instance params
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};


// 在这个Demo里暂时还是只有Opaque
enum class RenderLayer : int { Opaque = 0, Count };

class LandAndWavesApp : public D3DApp {
public:
    LandAndWavesApp(HINSTANCE hInstance);
    virtual void ReleaseResource() override;
    virtual bool Initialize() override;
    void Update(const GameTimer &timer) override;
    void Draw(const GameTimer &gt) override;

private:
    void BuildRootSignature();
    void BuildShaderAndInputLayout();
    void BuildLandGeometry();
    void BuildWavesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRenderItems();
    void BuildMaterials();
    void UpdateWaves(const GameTimer &gt);
    void UpdateMaterialCBS(const GameTimer &gt);

    void UpdateObjectCB(const GameTimer &timer);
    void UpdateMainPassCB(const GameTimer &timer);
    void DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<const RenderItem *> &ritems);
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    // std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
    ResourcePathSearcher mResourceManager;

    std::unordered_map<std::string, ComPtr<ID3D10Blob>> mShaders;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	// std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    using RenderItemLayer = std::vector<const RenderItem *>;
    std::array<RenderItemLayer, (int) RenderLayer::Count> mRitemLayers;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();
    int mCurrFrameResourceIndex = 0;

    FrameResource *mCurrFrameResource = nullptr;
    PassConstants mMainPassCB;
    bool mbShowWireFrame = false;
    bool mbVsync = true;

    UINT mWaveVerticesCount = 0;
};

inline LandAndWavesApp::LandAndWavesApp(HINSTANCE hInstance) : D3DApp(hInstance) {
    auto path = PD::ResourcePathSearcher::Path(PROJECT_DIR);
    if (!path.is_absolute()) { path = fs::absolute(path); }
    mResourceManager.add_path(path);
    mResourceManager.add_path(path / L"Shaders");
}

inline void LandAndWavesApp::ReleaseResource() { mRootSignature.Reset(); }

inline bool LandAndWavesApp::Initialize() {
    if (!D3DApp::Initialize()) { return false; }
    mCamera = new PD::Camera(SimpleMath::Vector3(0, 1, -5), SimpleMath::Vector3(0, 0, 1), SimpleMath::Vector3(0, 1, 0));
    assert(mCommandList);
    assert(mDirectCmdListAlloc);
    HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    // ready for record
    BuildRootSignature();
    BuildShaderAndInputLayout();
    BuildLandGeometry();
    BuildWavesGeometry();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // end of record
    HR(mCommandList->Close());
    std::array<ID3D12CommandList *, 1> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
    FlushCommandQueue();

    return true;
}

inline void LandAndWavesApp::Update(const GameTimer &timer) {

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
    UpdateMaterialCBS(timer);
    UpdateMainPassCB(timer);

    ImGuiPrepareDraw();
    static bool bShowDemoWindow = false;
    if (bShowDemoWindow) ImGui::ShowDemoWindow(&bShowDemoWindow);

    ImGui::Begin("LandScape!");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::SliderFloat("MoveSpeed", &mCamera->MovementSpeed, 0.0f, 50.0f);
    // ex 8.16.2
    if(ImGui::SliderFloat("GrassMatRoughness", &mMaterials["grass"].get()->Roughness,0.0,1.0))
    {
        mMaterials["grass"]->NumFramesDirty++;
    };
    ImGui::Checkbox("MSAA State", &GetMSAAState());
    ImGui::Checkbox("WireFrame", &mbShowWireFrame);
    ImGui::Checkbox("VSync", &mbVsync);
    ImGui::Checkbox("DemoWindow", &bShowDemoWindow);
    ImGui::End();
    ImGui::Render();
}

void LandAndWavesApp::UpdateObjectCB(const GameTimer &timer) {
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto &element : mAllRitems) {
        // upload data when needed
        if (element->NumFramesDirty > 0) {
            XMMATRIX world = XMLoadFloat4x4(&element->World);
            ObjectConstants objCB;
            XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objCB.InvTransWorld, XMMatrixTranspose(MathHelper::InverseTranspose(world)));
            currObjectCB->CopyData(element->ObjectCBIndex, objCB);
            element->NumFramesDirty--;
        }
    }
}
void LandAndWavesApp::UpdateMainPassCB(const GameTimer &gt) {
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
    mMainPassCB.AmbientLight = {0.25,0.25,0.25,1.0};

    // world
    auto LightDir = DirectX::SimpleMath::Vector3(-1, -1, 0);
    LightDir.Normalize();
    mMainPassCB.Lights[0].Direction = LightDir;
    mMainPassCB.Lights[0].Strength= XMFLOAT3(1,1,0.9f);

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

inline void LandAndWavesApp::DrawRenderItems(ID3D12GraphicsCommandList *cmdList,
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
        auto MatCBAddress = MatCBStartAddress+ ri->Mat->MatCBIndex* matCBByteSize;
        cmdList->SetGraphicsRootConstantBufferView(0, ObjCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(1, MatCBAddress);
        assert(ri->IndexCount > 0);
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

void LandAndWavesApp::Draw(const GameTimer &gt) {


    // 取出当前帧的Alloc,然后把cmdlist关联到这个alloc
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    HR(cmdListAlloc->Reset());

    auto SwitchPSO = [this]() {
        if (GetMSAAState()) { return mMSAAOpaquePSO.Get(); }
        return mbShowWireFrame ? mPSOs["opaque_wireframe"].Get() : mPSOs["opaque"].Get();
    };
    HR(mCommandList->Reset(cmdListAlloc.Get(), SwitchPSO()));

    auto MsaaRTVHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), 2, mRtvDescriptorSize);
    auto MsaaDSVHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1, mDsvDescriptorSize);

    if (GetMSAAState()) {
        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMSAART.Get(),
                                                                               D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET));
        mCommandList->ClearRenderTargetView(MsaaRTVHandle, DirectX::Colors::LightBlue, 0, nullptr);
        mCommandList->ClearDepthStencilView(MsaaDSVHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0,
                                            0, nullptr);
    }

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);


    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0,
                                        0, nullptr);

    if (GetMSAAState()) {
        mCommandList->OMSetRenderTargets(1, &MsaaRTVHandle, true, &MsaaDSVHandle);
    } else {
        mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    }


    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetGraphicsRootConstantBufferView(2, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRitemLayers[(int) RenderLayer::Opaque]);

    if (GetMSAAState()) {
        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMSAART.Get(),
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                               D3D12_RESOURCE_STATE_RESOLVE_SOURCE));


        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                               D3D12_RESOURCE_STATE_RESOLVE_DEST));
        mCommandList->ResolveSubresource(CurrentBackBuffer(), 0, mMSAART.Get(), 0, mBackBufferFormat);


        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                               D3D12_RESOURCE_STATE_RESOLVE_DEST,
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET));
    }

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
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


inline void LandAndWavesApp::BuildRootSignature() {

    CD3DX12_ROOT_PARAMETER slotRootParameter[3];
    // 根签名需要3个ConstantBufferView，但是不用放在堆里
    // 一个放Object，一个放PassCB,一个放MatCB
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr,
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

inline void LandAndWavesApp::BuildShaderAndInputLayout() {
    spdlog::info("Bulding Shaders");
    auto ShaderPath = mResourceManager.find_path("color.hlsl");

    auto VSBlob = CompileShader(ShaderPath, nullptr, "VSMain", "vs_5_0");
    auto PSBlob = CompileShader(ShaderPath, nullptr, "PSMain", "ps_5_0");
    assert(VSBlob);
    assert(PSBlob);
    mShaders["standardVS"] = VSBlob;
    mShaders["opaquePS"] = PSBlob;

    spdlog::info("Bulding Input Layout");
    mInputLayout = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
             0},
    };
}

inline void LandAndWavesApp::BuildLandGeometry() {
    spdlog::info("Bulding LandScape Geometries");

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);
    GeometryGenerator::MeshData geoSphere = geoGen.CreateGeosphere(2.0, 3);

    std::vector<Vertex> vertices(grid.Vertices.size() + geoSphere.Vertices.size());

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
    for (size_t i = 0; i < grid.Vertices.size(); ++i,++k) {
        auto &p = grid.Vertices[k].Position;
        vertices[k].Pos = p;
        vertices[k].Pos.y = HeightFunc(p.x, p.z);
        vertices[k].Normal = NormalFunc(p.x,p.z);

    }

    for (size_t i = 0; i < geoSphere.Vertices.size(); ++i,++k) {
        vertices[k].Pos = geoSphere.Vertices[i].Position;
        vertices[k].Normal = geoSphere.Vertices[i].Normal;
    }


    std::vector<uint16_t> indices;
    indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
    // ex 7.9.1 create GeoSphere
    indices.insert(indices.end(), geoSphere.GetIndices16().begin(), geoSphere.GetIndices16().end());

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

    SubmeshGeometry GeoSphereSubmesh;
    GeoSphereSubmesh.BaseVertexLocation = grid.Vertices.size();
    GeoSphereSubmesh.StartIndexLocation = grid.Indices32.size();
    GeoSphereSubmesh.IndexCount = geoSphere.Indices32.size();

    geo->DrawArgs["grid"] = gridSubmesh;
    geo->DrawArgs["geoSphere"] = GeoSphereSubmesh;

    mGeometries[geo->name] = std::move(geo);
}

inline void LandAndWavesApp::BuildWavesGeometry() {

    spdlog::info("Bulding Waves Geometries");

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData waves = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

    std::vector<Vertex> vertices(waves.Vertices.size());

    for (size_t i = 0; i < waves.Vertices.size(); ++i) {
        auto &p = waves.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Normal = waves.Vertices[i].Normal;
    }

    const UINT vbByteSize = (UINT) vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT) waves.Indices32.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "waveGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), waves.GetIndices16().data(), ibByteSize);

    // 这里不用上传顶点数据，因为后面我们会在UpdateWaves里动态生成顶点数据
    // 正式场景里不要用这种方法，太蠢

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
    waveSubmesh.IndexCount = waves.Indices32.size();
    mWaveVerticesCount = vertices.size();

    geo->DrawArgs["wave"] = waveSubmesh;

    mGeometries[geo->name] = std::move(geo);
}

inline void LandAndWavesApp::BuildPSOs() {

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

    spdlog::info("Building WireFrame PSO");
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));

    spdlog::info("Building MSAA PSO");
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.SampleDesc.Count = 4;
    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mMSAAOpaquePSO)));
}

inline void LandAndWavesApp::BuildFrameResources() {
    for (int i = 0; i < kNumFrameResources; i++) {
        mFrameResources.push_back(
                std::make_unique<FrameResource>(mD3dDevice.Get(), 1, (UINT) mAllRitems.size(), mMaterials.size(), mWaveVerticesCount));
    }
}

inline void LandAndWavesApp::BuildRenderItems() {
    spdlog::info("Build Render Items");

    int ObjectCBIndex = 0;
    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
    gridRitem->ObjectCBIndex = ObjectCBIndex++;
    gridRitem->Geo = mGeometries["landGeo"].get();
    gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->Mat = mMaterials["grass"].get();

    RenderItem *sphereItems[2];
    for (int i = 0; i < 2; i++) {
        auto geoSphereRitem = new RenderItem();
        sphereItems[i] = geoSphereRitem;
        auto WorldMatrix = DirectX::XMMatrixTranslation(i * 20.0, 15.0, 10.0);
        XMStoreFloat4x4(&geoSphereRitem->World, WorldMatrix);
        geoSphereRitem->ObjectCBIndex = ObjectCBIndex++;
        geoSphereRitem->Geo = mGeometries["landGeo"].get();
        geoSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        geoSphereRitem->IndexCount = geoSphereRitem->Geo->DrawArgs["geoSphere"].IndexCount;
        geoSphereRitem->StartIndexLocation = geoSphereRitem->Geo->DrawArgs["geoSphere"].StartIndexLocation;
        geoSphereRitem->BaseVertexLocation = geoSphereRitem->Geo->DrawArgs["geoSphere"].BaseVertexLocation;
    }
    sphereItems[0]->Mat = mMaterials["grass"].get();
    sphereItems[1]->Mat = mMaterials["water"].get();

    auto waveRitem = std::make_unique<RenderItem>();
    waveRitem->World = MathHelper::Identity4x4();
    waveRitem->ObjectCBIndex = ObjectCBIndex++;
    waveRitem->Geo = mGeometries["waveGeo"].get();
    waveRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    waveRitem->IndexCount = waveRitem->Geo->DrawArgs["wave"].IndexCount;
    waveRitem->StartIndexLocation = waveRitem->Geo->DrawArgs["wave"].StartIndexLocation;
    waveRitem->BaseVertexLocation = waveRitem->Geo->DrawArgs["wave"].BaseVertexLocation;
    waveRitem->Mat = mMaterials["water"].get();

    mRitemLayers.at((int) RenderLayer::Opaque).push_back(gridRitem.get());
    for (int i = 0; i < 2; i++) {
        mRitemLayers.at((int) RenderLayer::Opaque).push_back(sphereItems[i]);
        mAllRitems.push_back(std::unique_ptr<RenderItem>(sphereItems[i]));
    }
    mRitemLayers.at((int) RenderLayer::Opaque).push_back(waveRitem.get());

    mAllRitems.push_back(std::move(gridRitem));
    mAllRitems.push_back(std::move(waveRitem));
}

inline void LandAndWavesApp::BuildMaterials() {
    auto grassMat = std::make_unique<Material>();
    grassMat->DiffuseAlbedo = XMFLOAT4(0.2f,0.6f,0.2f,1.0f);
    grassMat->FresnelR0 = XMFLOAT3(0.01f,0.01f,0.01f);
    grassMat->Roughness = 0.125;
    grassMat->MatCBIndex = 0;
    grassMat->Name = "grass";

    auto waterMat = std::make_unique<Material>();
    waterMat->DiffuseAlbedo = XMFLOAT4(0.0f,0.1f,0.8f,1.0f);
    waterMat->FresnelR0 = XMFLOAT3(0.01f,0.01f,0.01f);
    waterMat->Roughness = 0;
    waterMat->MatCBIndex = 1;
    waterMat->Name = "water";

    mMaterials["grass"] = std::move(grassMat);
    mMaterials["water"] = std::move(waterMat);
}

// 这个方法属实很蠢
// 每一帧的时候更新所有的顶点位置。
// 正式场景不要用
inline void LandAndWavesApp::UpdateWaves(const GameTimer &gt) {

    auto WaveGeo = mGeometries["waveGeo"].get();
    auto currWavesVB = mCurrFrameResource->WavesUploader.get();
    auto WaveVerterices = (Vertex *) WaveGeo->VertexBufferCPU->GetBufferPointer();

    for (UINT i = 0; i < mWaveVerticesCount; ++i) {

        Vertex v;

        v.Pos = WaveVerterices[i].Pos;
        v.Pos.y = sin(gt.TotalTime() * 0.1f);
        v.Normal = WaveVerterices[i].Normal;
        currWavesVB->CopyData(i, v);
    }

    // Set the dynamic VB of the wave renderitem to the current frame VB.
    WaveGeo->VertexBufferGPU = currWavesVB->Resource();
}

inline void LandAndWavesApp::UpdateMaterialCBS(const GameTimer &gt) {
    auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
    for (auto& element: mMaterials) {
        // upload data when needed
        Material* mat = element.second.get();
        if (mat->NumFramesDirty > 0) {
            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);
            mat->NumFramesDirty--;
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    LandAndWavesApp theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}