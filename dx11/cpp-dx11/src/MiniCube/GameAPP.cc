#ifndef GAMEAPP_H
#define GAMEAPP_H
#include <d3dApp.hh>
#include <d3dUtils.hh>
#include <winnt.h>

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_glfw.h>
#include <vector>
#include <vertexLayout.hh>

namespace PD {

class GameApp : public D3DApp {
public:
  GameApp() : D3DApp() { this->WinTitle_ = "MiniCube"; };
  ~GameApp(){};

  bool Init() override {
    auto res = D3DApp::Init();
    if (!res)
      return res;
    if (!InitEffect())
      return false;
    if (!InitResource())
      return false;
    return true;
  }
  void glfw_keycallback(int key, int scancode, int action, int mods) override {
    ignore(scancode);
    ignore(mods);
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
      vsync_ = !vsync_;
    }
  }
  void DrawImGUI() override {
    D3DApp::DrawImGUI();
    {
      ImGui::Begin("Others!");
      ImGui::LabelText("Current Speed:", "%f", rotation_speed_);
      if (ImGui::Button("reload shaders")) {
        this->reloadShaders();
      }
      ImGui::Checkbox("vsync", &vsync_);
      ImGui::Checkbox("ReverseZ", &reverse_z_);
      ImGui::End();
    }
  }
  void ProcessInput(GLFWwindow *window) override {
    D3DApp::ProcessInput(window);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      this->rotation_speed_ += 0.01f * imgui_io_->DeltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      this->rotation_speed_ -= 0.01f * imgui_io_->DeltaTime;
  }
  void UpdateScene(float dt) override {
    static float phi = 0.0f, theta = 0.0f;
    phi += rotation_speed_ * dt, theta += rotation_speed_ * dt;
    CBuffer_.World =
        DirectX::XMMatrixRotationX(phi) * DirectX::XMMatrixRotationY(theta);
    CBuffer_.Proj = GetProjectionMatrixFovLH(DirectX::XM_PIDIV4, AspectRatio(),
                                             0.1f, 100.0f, this->reverse_z_);

// Transpose all data
//在hlsl里使用列主元存储矩阵，因为向量右乘以矩阵，所以一次要取矩阵的一个列
//在C++里矩阵是行主元，从行主元到列主元相当于经过了一次转置
//所以在C++里再手动转置一下，抵消上面的一次转置
#ifndef OPENGL_MATRIX
    CBuffer_.World = DirectX::XMMatrixTranspose(CBuffer_.World);
    CBuffer_.Proj = DirectX::XMMatrixTranspose(CBuffer_.Proj);
#endif
    // map data
    D3D11_MAPPED_SUBRESOURCE mappedData;
    HRESULT hr = S_OK;
    HR(pd3dDeviceIMContext_->Map(pCBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                                 &mappedData));
    std::memcpy(mappedData.pData, &CBuffer_, sizeof CBuffer_);
    pd3dDeviceIMContext_->Unmap(pCBuffer_.Get(), 0);
  };
  void DrawScene() override {
    assert(pd3dDeviceIMContext_);
    assert(pSwapChain_);
    float dt = imgui_io_->DeltaTime;
    UpdateScene(dt * 1000.0f);
    static float blue[4] = {0.0f, 0.0f, 0.2f, 1.0f};
    pd3dDeviceIMContext_->ClearRenderTargetView(pRenderTargetView_.Get(), blue);
    pd3dDeviceIMContext_->ClearDepthStencilView(
        pDepthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        reverse_z_ ? 0.0f : 1.0f, 0);
    pd3dDeviceIMContext_->DrawIndexed(36, 0, 0);
  };

private:
  ComPtr<ID3D11InputLayout> pVertexLayout_;
  ComPtr<ID3D11Buffer> pVertexBuffer_;
  ComPtr<ID3D11Buffer> pVertexIndexBuffer_;
  ComPtr<ID3D11Buffer> pCBuffer_;
  ComPtr<ID3D11VertexShader> pVertexShader_;
  ComPtr<ID3D11PixelShader> pPixelShader_;
  struct MVP {
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Proj;
  };
  MVP CBuffer_;
  float rotation_speed_ = 0.0001f;

protected:
  bool reloadShaders() {
    ComPtr<ID3DBlob> blob;
    HRESULT hr;
    ComPtr<ID3D11VertexShader> vshader;
    ComPtr<ID3D11PixelShader> pshader;
    // force relaod
    // if failed ,nothing happens, return false immediately

#ifndef OPENGL_MATRIX
    HR_RETURN(CreateShaderFromFile(L"HLSL\\Triangle_VS.cso",
                                   L"HLSL\\Triangle_VS.hlsl", "VS", "vs_5_0",
                                   blob.ReleaseAndGetAddressOf(), true));
#else
    HR(CreateShaderFromFile(L"HLSL\\Triangle_VS_OpenGL.cso",
                            L"HLSL\\Triangle_VS_OpenGL.hlsl", "VS", "vs_5_0",
                            blob.ReleaseAndGetAddressOf()));
#endif
    HR(pd3dDevice_->CreateVertexShader(blob->GetBufferPointer(),
                                       blob->GetBufferSize(), nullptr,
                                       vshader.GetAddressOf()));
    SAFE_RELEASE(pVertexShader_);
    SAFE_RELEASE(pVertexLayout_);
    pVertexShader_ = vshader;

    HR(pd3dDevice_->CreateInputLayout(
        VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        pVertexLayout_.GetAddressOf()));
    HR_RETURN(CreateShaderFromFile(L"HLSL\\Triangle_PS.cso",
                                   L"HLSL\\Triangle_PS.hlsl", "PS", "ps_5_0",
                                   blob.ReleaseAndGetAddressOf(), true));

    SAFE_RELEASE(pPixelShader_);
    HR(pd3dDevice_->CreatePixelShader(blob->GetBufferPointer(),
                                      blob->GetBufferSize(), nullptr,
                                      pshader.GetAddressOf()));
    pPixelShader_ = pshader;

    pd3dDeviceIMContext_->VSSetShader(pVertexShader_.Get(), nullptr, 0);
    pd3dDeviceIMContext_->PSSetShader(pPixelShader_.Get(), nullptr, 0);
    return true;
  }
  bool InitEffect() {
    ComPtr<ID3DBlob> blob;
    HRESULT hr;

#ifndef OPENGL_MATRIX
    HR(CreateShaderFromFile(L"HLSL\\Triangle_VS.cso", L"HLSL\\Triangle_VS.hlsl",
                            "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
#else
    HR(CreateShaderFromFile(L"HLSL\\Triangle_VS_OpenGL.cso",
                            L"HLSL\\Triangle_VS_OpenGL.hlsl", "VS", "vs_5_0",
                            blob.ReleaseAndGetAddressOf()));
#endif
    HR(pd3dDevice_->CreateVertexShader(blob->GetBufferPointer(),
                                       blob->GetBufferSize(), nullptr,
                                       pVertexShader_.GetAddressOf()));
    HR(pd3dDevice_->CreateInputLayout(
        VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(),
        pVertexLayout_.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\Triangle_PS.cso", L"HLSL\\Triangle_PS.hlsl",
                            "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pd3dDevice_->CreatePixelShader(blob->GetBufferPointer(),
                                      blob->GetBufferSize(), nullptr,
                                      pPixelShader_.GetAddressOf()));

    return true;
  }
  bool InitResource() {
    using namespace DirectX;

    VertexPosColor vertices[] = {
        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)}};

    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.ByteWidth = sizeof vertices;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    HRESULT hr;

    HR(pd3dDevice_->CreateBuffer(&vertexBufferDesc, &InitData,
                                 pVertexBuffer_.GetAddressOf()));
    //索引缓冲区描述
    uint32_t indices[] = {// 正面
                          0, 1, 2, 2, 3, 0,
                          // 左面
                          4, 5, 1, 1, 0, 4,
                          // 顶面
                          1, 5, 6, 6, 2, 1,
                          // 背面
                          7, 6, 5, 5, 4, 7,
                          // 右面
                          3, 2, 6, 6, 7, 3,
                          // 底面
                          4, 0, 3, 3, 7, 4};
    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.ByteWidth = sizeof indices;
    // 申请缓冲区
    InitData.pSysMem = indices;
    HR(pd3dDevice_->CreateBuffer(&indexBufferDesc, &InitData,
                                 pVertexIndexBuffer_.GetAddressOf()));
    // 申请cbuffer
    D3D11_BUFFER_DESC CBufferDesc;
    ZeroMemory(&CBufferDesc, sizeof CBufferDesc);
    CBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    CBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    CBufferDesc.ByteWidth = sizeof(MVP);
    HR(pd3dDevice_->CreateBuffer(&CBufferDesc, nullptr,
                                 pCBuffer_.GetAddressOf()));
    CBuffer_.World = XMMatrixIdentity();
    CBuffer_.View = XMMatrixLookAtLH(XMVectorSet(0.0, 0.0, -5.0, 0.0),
                                     XMVectorSet(0.0, 0.0, 0.0, 0.0),
                                     XMVectorSet(0.0, 1.0, 0.0, 0.0));
#ifndef OPENGL_MATRIX
    CBuffer_.View = DirectX::XMMatrixTranspose(CBuffer_.View);
#endif

    //绑定资源
    UINT stride = sizeof(VertexPosColor);
    UINT offset = 0;

    pd3dDeviceIMContext_->IASetVertexBuffers(
        0, 1, pVertexBuffer_.GetAddressOf(), &stride, &offset);
    // 设置图元类型，设定输入布局
    pd3dDeviceIMContext_->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dDeviceIMContext_->IASetInputLayout(pVertexLayout_.Get());
    pd3dDeviceIMContext_->IASetIndexBuffer(pVertexIndexBuffer_.Get(),
                                           DXGI_FORMAT_R32_UINT, 0);
    // 将着色器绑定到渲染管线
    pd3dDeviceIMContext_->VSSetShader(pVertexShader_.Get(), nullptr, 0);
    pd3dDeviceIMContext_->VSSetConstantBuffers(0, 1, pCBuffer_.GetAddressOf());
    pd3dDeviceIMContext_->PSSetShader(pPixelShader_.Get(), nullptr, 0);

    D3D11SetDebugObjectName(pVertexLayout_.Get(), "VertexPosColorLayout");
    D3D11SetDebugObjectName(pVertexBuffer_.Get(), "VertexBuffer");
    D3D11SetDebugObjectName(pVertexShader_.Get(), "Trangle_VS");
    D3D11SetDebugObjectName(pPixelShader_.Get(), "Trangle_PS");

    return true;
  };
};
} // namespace PD
int main() {
  PD::GameApp app;
  app.Init();
  app.Run();
  return 0;
}

#endif
