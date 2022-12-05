#include "MathHelper.h"
#include "UploadBuffer.hh"
#include "resource_path_searcher.h"

#include <GeometryGenerator.h>
#include <array>
#include <d3dApp.hh>
#include <spdlog/spdlog.h>

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"


using namespace PD;

using namespace DirectX;
using DirectX::PackedVector::XMCOLOR;
struct Vertex
{
    XMFLOAT3 Pos;
    // XMFLOAT4 Color;

    // Chapter 6.13 ex10 packed COLOR
    // ARGB8
    // in DXGI_FORMAT: BGRA8
    // DXGI is in little-0Endine
    XMCOLOR Color;
};

struct ObjectConstants
{

    XMFLOAT4X4 MVP = MathHelper::Identity4x4();
    float gTime = 0;
};


class ShapesAPP : public D3DApp {
public:
    ShapesAPP(HINSTANCE hInstance);
    virtual void ReleaseResource() override;
    virtual bool Initialize() override;
    void Update(const GameTimer &timer) override;
    void Draw(const GameTimer &gt) override;
    private:

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShaderAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();
    ComPtr<ID3D12DescriptorHeap> mCbvHeap;
    ComPtr<ID3D12PipelineState> mPSO;
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
    ResourcePathSearcher mResourceManager;

    ComPtr<ID3DBlob> mvsByteCode;
    ComPtr<ID3DBlob> mpsByteCode;

    std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};

inline ShapesAPP::ShapesAPP(HINSTANCE hInstance) : D3DApp(hInstance) {
    auto path = PD::ResourcePathSearcher::Path(PROJECT_DIR);
    if (!path.is_absolute()) { path = fs::absolute(path); }
    mResourceManager.add_path(path);
    mResourceManager.add_path(path / L"Shaders");
}

inline void ShapesAPP::ReleaseResource() {
    mRootSignature.Reset();
    mPSO.Reset();
    mCbvHeap.Reset();

    mvsByteCode.Reset();
    mpsByteCode.Reset();

    mBoxGeo.reset();
    mObjectCB.reset();
}

inline bool ShapesAPP::Initialize() {
    if(!D3DApp::Initialize())
    {
        return false;
    }
    assert(mCommandList);
    assert(mDirectCmdListAlloc);
    HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    // ready for record
    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShaderAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    // end of record
    HR(mCommandList->Close());
    std::array<ID3D12CommandList*,1> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
    FlushCommandQueue();
    // after flush, it's safe to release uploader resource
    mBoxGeo->DisposeUploaders();

    return true;
}

inline void ShapesAPP::Update(const GameTimer &timer) {

    XMVECTOR pos = XMVectorSet(0, 0, -5, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    float aspect = GetAspectRatio();
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspect, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

    static float phi = 0.0f, theta = 0.0f;
    static float rotationSpeed = 0.1f;
    phi += rotationSpeed * timer.DeltaTime(), theta += rotationSpeed * timer.DeltaTime();

    XMMATRIX world = DirectX::XMMatrixRotationX(phi) * DirectX::XMMatrixRotationY(theta);
    XMStoreFloat4x4(&mWorld, world);
    // XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world * view * proj;

    // Update the constant buffer with the latest worldViewProj matrix.
    ObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.MVP, XMMatrixTranspose(worldViewProj));

    objConstants.gTime = timer.TotalTime();
    mObjectCB->CopyData(0, objConstants);

    ImGuiPrepareDraw();
    static bool flag = true;
    if (flag) ImGui::ShowDemoWindow(&flag);

    ImGui::Begin("Hello, world!");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::SliderFloat("RotationSpeed", &rotationSpeed, 0.0f, 1.0f);
    ImGui::Checkbox("MSAA State", &GetMSAAState());
    ImGui::End();
    ImGui::Render();
}

void ShapesAPP::Draw(const GameTimer &gt) {


    HR(mDirectCmdListAlloc->Reset());
    HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), GetMSAAState() ? mMSAAPSO.Get() : mPSO.Get()));

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


    std::vector<ID3D12DescriptorHeap *> descriptorHeaps{mCbvHeap.Get()};
    mCommandList->SetDescriptorHeaps(descriptorHeaps.size(), descriptorHeaps.data());
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
    mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
    // mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    // test: root signature
    mCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    for (auto [key, value] : mBoxGeo->DrawArgs) {
        mCommandList->DrawIndexedInstanced(value.IndexCount, 1, value.StartIndexLocation, value.BaseVertexLocation, 0);
    }


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
    mCommandList->SetPipelineState(mPSO.Get());
    mCommandList->SetDescriptorHeaps(1, mImGuiCbvHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());


    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                           D3D12_RESOURCE_STATE_PRESENT));
    HR(mCommandList->Close());
    std::vector<ID3D12CommandList *> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());

    // turn vsync on
    HR(mSwapChain->Present(1, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % kSwapChainBufferCount;
    FlushCommandQueue();
}

inline void ShapesAPP::BuildDescriptorHeaps() {
    assert(mRtvHeap.Get());
    assert(mDsvHeap.Get());
    spdlog::info("Building Descriptor Heaps");
    // 除了父类建立的 rtv,Dsv以外还需要建立 src/uav/cbv的描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

inline void ShapesAPP::BuildConstantBuffers() {
    spdlog::info("Building Constant Buffers");
    assert(!mObjectCB);
    mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(mD3dDevice.Get(), 1, true);

    uint32_t ObjSize = CalcConstantBufferBytesSize(sizeof(ObjectConstants));

    // 计算这个物体在整个cb内的偏移量，因为这个例子只有一个物体，所以偏移量是0
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * ObjSize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = ObjSize;

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(mCbvHeap->GetCPUDescriptorHandleForHeapStart(), 0,
                                            mCbvSrvUavDescriptorSize);
    mD3dDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
}

inline void ShapesAPP::BuildRootSignature() {
    // 可以有多个root parameter，对应函数的多个形参数?
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // 用cbvTable初始化 root parameter
    // 这个参数包含1个 cbv descriptor
    // CD3DX12_DESCRIPTOR_RANGE cbvTable;
    // cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    // slotRootParameter->InitAsDescriptorTable(1, &cbvTable);

    // test: root descriptor
    slotRootParameter[0].InitAsConstantBufferView(0, 0);

    //TODO: flag
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
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

inline void ShapesAPP::BuildShaderAndInputLayout() {
    spdlog::info("Bulding Shaders");
    auto ShaderPath = mResourceManager.find_path("color.hlsl");
    mvsByteCode = CompileShader(ShaderPath, nullptr, "VS", "vs_5_0");
    mpsByteCode = CompileShader(ShaderPath, nullptr, "PS", "ps_5_0");
    assert(mvsByteCode);
    assert(mpsByteCode);
    spdlog::info("Bulding Input Layout");
    mInputLayout = {
            {"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}

inline void ShapesAPP::BuildBoxGeometry() {
    spdlog::info("Bulding BoxGemotries");


    // Chapter 6.13 ex10 packed COLOR

    std::array<Vertex, 8> BoxVertices = {
            Vertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMCOLOR(Colors::White)}),
            Vertex({XMFLOAT3(-1.0f, +1.0f, -1.0f), XMCOLOR(Colors::Black)}),
            Vertex({XMFLOAT3(+1.0f, +1.0f, -1.0f), XMCOLOR(Colors::Red)}),
            Vertex({XMFLOAT3(+1.0f, -1.0f, -1.0f), XMCOLOR(Colors::Green)}),
            Vertex({XMFLOAT3(-1.0f, -1.0f, +1.0f), XMCOLOR(Colors::Blue)}),
            Vertex({XMFLOAT3(-1.0f, +1.0f, +1.0f), XMCOLOR(Colors::Yellow)}),
            Vertex({XMFLOAT3(+1.0f, +1.0f, +1.0f), XMCOLOR(Colors::Cyan)}),
            Vertex({XMFLOAT3(+1.0f, -1.0f, +1.0f), XMCOLOR(Colors::Magenta)}),

    };

    std::array<std::uint16_t, 36> BoxIndices = {// front face
                                                0, 1, 2, 0, 2, 3,

                                                // back face
                                                4, 6, 5, 4, 7, 6,

                                                // left face
                                                4, 5, 1, 4, 1, 0,

                                                // right face
                                                3, 2, 6, 3, 6, 7,

                                                // top face
                                                1, 5, 6, 1, 6, 2,

                                                // bottom face
                                                4, 0, 3, 4, 3, 7};

    // Chapter 6.13 ex10 packed COLOR
    std::array<Vertex, 3> TriVertices = {
            Vertex({XMFLOAT3(-2.0f, -2.0f, 1.0f), XMCOLOR(Colors::Red)}),
            Vertex({XMFLOAT3(2.0f, -2.0f, 1.0f), XMCOLOR(Colors::Blue)}),
            Vertex({XMFLOAT3(0.0f, 2.0f, 1.0f), XMCOLOR(Colors::Green)}),
    };

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);

    std::vector<::Vertex> sphereVertices;
    for (auto v : sphere.Vertices) {
        ::Vertex vertex;
        vertex.Pos = v.Position;
        vertex.Pos.x += 1.7f;
        vertex.Color = XMCOLOR(Colors::Silver);
        sphereVertices.push_back(vertex);
    }
    // 对于Dx，顺时针是front face
    // 对于openGL而言 front face是ccw
    std::array<std::uint16_t, 3> TriIndices = {0, 2, 1};
    std::vector<Vertex> vertices;
    vertices.insert(vertices.end(), BoxVertices.begin(), BoxVertices.end());
    vertices.insert(vertices.end(), TriVertices.begin(), TriVertices.end());
    vertices.insert(vertices.end(), sphereVertices.begin(), sphereVertices.end());
    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), BoxIndices.begin(), BoxIndices.end());
    indices.insert(indices.end(), TriIndices.begin(), TriIndices.end());
    indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());


    const UINT vbBytesSize = (UINT) vertices.size() * sizeof(Vertex);
    const UINT ibBytesSize = (UINT) indices.size() * sizeof(uint16_t);
    assert(!mBoxGeo);
    mBoxGeo = std::make_unique<MeshGeometry>();
    mBoxGeo->name = "BoxGeo";

    spdlog::info("Building Box VBO/IBO CPU");
    HR(D3DCreateBlob(vbBytesSize, mBoxGeo->VertexBufferCPU.GetAddressOf()));
    CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbBytesSize);

    HR(D3DCreateBlob(ibBytesSize, mBoxGeo->IndexBufferCPU.GetAddressOf()));
    CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibBytesSize);

    spdlog::info("Building Box VBO/IBO GPU");
    mBoxGeo->VertexBufferGPU = CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), vertices.data(), vbBytesSize,
                                                   mBoxGeo->VertexBufferUploader);

    mBoxGeo->IndexBufferGPU = CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), indices.data(), ibBytesSize,
                                                  mBoxGeo->IndexBufferUploader);
    mBoxGeo->VertexBytesStride = sizeof(Vertex);
    mBoxGeo->VertexBufferByteSize = vbBytesSize;
    mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    mBoxGeo->IndexBufferBytesSize = ibBytesSize;

    spdlog::info("Building Box Submesh");
    SubmeshGeometry submesh{};
    submesh.BaseVertexLocation = 0;
    submesh.IndexCount = (UINT) BoxIndices.size();
    submesh.StartIndexLocation = 0;
    mBoxGeo->DrawArgs["box"] = submesh;


    spdlog::info("Building Tri Submesh");
    submesh.BaseVertexLocation = BoxVertices.size();
    submesh.IndexCount = (UINT) TriIndices.size();
    submesh.StartIndexLocation = BoxIndices.size();
    mBoxGeo->DrawArgs["tri"] = submesh;

    spdlog::info("Building Sphere Submesh");
    submesh.BaseVertexLocation = BoxVertices.size() + TriVertices.size();
    submesh.IndexCount = (UINT) sphere.Indices32.size();
    submesh.StartIndexLocation = BoxIndices.size() + TriIndices.size();
    mBoxGeo->DrawArgs["sphere"] = submesh;
}

inline void ShapesAPP::BuildPSO() {

    spdlog::info("Building PSO");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{0};
    psoDesc.InputLayout = {mInputLayout.data(), (UINT) mInputLayout.size()};
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = {(void *) (mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize()};
    psoDesc.PS = {(void *) (mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize()};

    // RasterzeState里的MultiSampleEnabled只与 line rendering有关?
    // MultiSampleEnabled: Set to TRUE to use the quadrilateral line anti-aliasing algorithm and to FALSE to use the alpha line anti-aliasing algorithm.
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

    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));

    spdlog::info("Building MSAA PSO");
    psoDesc.SampleDesc.Count = 4;
    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mMSAAPSO)));
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    ShapesAPP theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}