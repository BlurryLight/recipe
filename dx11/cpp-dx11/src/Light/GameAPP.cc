#ifndef GAMEAPP_H
#define GAMEAPP_H
#include <d3dApp.hh>
#include <d3dUtils.hh>
#include <winnt.h>

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_glfw.h>
#include <resource_path_searcher.h>
#include <shapes.hh>
#include <vertexLayout.hh>
namespace PD {

class GameApp : public D3DApp {
public:
  GameApp() : D3DApp() {
    this->WinTitle_ = "Light Example";
    // project_dir is defined in cmake
    // don't use __FILE__. The value is changed between different generator
    auto path = PD::ResourcePathSearcher::Path(PROJECT_DIR);
    if (!path.is_absolute()) {
      path = fs::absolute(path);
    }
    path_manager_.add_path(path);
    dir_path_ = path;
    path_manager_.add_path(path / "HLSL");
    auto resource_path = ResourcePathSearcher::root_path / "resources";
    path_manager_.add_path(resource_path / "models");
  };
  ~GameApp(){};

  bool Init() override {
    auto res = D3DApp::Init();
    auto key_cb = [](GLFWwindow *win, int key, int scancode, int action,
                     int mods) {
      static_cast<D3DApp *>(glfwGetWindowUserPointer(win))
          ->glfw_keycallback(key, scancode, action, mods);
    };
    glfwSetKeyCallback(window_, key_cb);
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
                                             0.1f, 100.0f, reverse_z_);
    CBuffer_.WorldInverseTranspose = DirectX::XMMatrixInverse(
        nullptr, DirectX::XMMatrixTranspose(CBuffer_.World));
    CBuffer_.WorldInverseTranspose =
        DirectX::XMMatrixTranspose(CBuffer_.WorldInverseTranspose);
    CBuffer_.World = DirectX::XMMatrixTranspose(CBuffer_.World);
    CBuffer_.Proj = DirectX::XMMatrixTranspose(CBuffer_.Proj);
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
    // for (const auto &shape : shapes_) {
    //   shape->draw(pd3dDeviceIMContext_.Get());
    // }
    // cube_->draw();
    models_[0]->draw();
  };

private:
  ComPtr<ID3D11InputLayout> pVertexLayout_;
  ComPtr<ID3D11Buffer> pCBuffer_;
  ComPtr<ID3D11VertexShader> pVertexShader_;
  ComPtr<ID3D11PixelShader> pPixelShader_;
  struct MVP {
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX WorldInverseTranspose;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Proj;
  };
  MVP CBuffer_;
  float rotation_speed_ = 0.0001f;
  std::vector<std::unique_ptr<Model>> models_;
  std::unique_ptr<CubeMesh> cube_;
  ResourcePathSearcher path_manager_;
  ResourcePathSearcher::Path dir_path_;

protected:
  bool reloadShaders() {
    ComPtr<ID3DBlob> blob;
    HRESULT hr;
    ComPtr<ID3D11VertexShader> vshader;
    ComPtr<ID3D11PixelShader> pshader;
    // force relaod
    // if failed ,nothing happens, return false immediately
    HR_RETURN(CreateShaderFromFile(
        (dir_path_ / "HLSL" / "Triangle_VS.cso").wstring(),
        path_manager_.find_path("Triangle_VS.hlsl").wstring(), "VS", "vs_5_0",
        blob.ReleaseAndGetAddressOf(), true));
    HR(pd3dDevice_->CreateVertexShader(blob->GetBufferPointer(),
                                       blob->GetBufferSize(), nullptr,
                                       vshader.GetAddressOf()));
    SAFE_RELEASE(pVertexShader_);
    SAFE_RELEASE(pVertexLayout_);
    pVertexShader_ = vshader;

    HR(pd3dDevice_->CreateInputLayout(
        VertexPosNormalTex::inputLayout,
        ARRAYSIZE(VertexPosNormalTex::inputLayout), blob->GetBufferPointer(),
        blob->GetBufferSize(), pVertexLayout_.GetAddressOf()));
    HR_RETURN(CreateShaderFromFile(
        (dir_path_ / "HLSL" / "Triangle_PS.cso").wstring(),
        path_manager_.find_path("HLSL\\Triangle_PS.hlsl").wstring(), "PS",
        "ps_5_0", blob.ReleaseAndGetAddressOf(), true));

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

    HR(CreateShaderFromFile(
        (dir_path_ / "HLSL" / "Triangle_VS.cso").wstring(),
        path_manager_.find_path("HLSL\\Triangle_VS.hlsl").wstring(), "VS",
        "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pd3dDevice_->CreateVertexShader(blob->GetBufferPointer(),
                                       blob->GetBufferSize(), nullptr,
                                       pVertexShader_.GetAddressOf()));
    HR(pd3dDevice_->CreateInputLayout(
        VertexPosNormalTex::inputLayout,
        ARRAYSIZE(VertexPosNormalTex::inputLayout), blob->GetBufferPointer(),
        blob->GetBufferSize(), pVertexLayout_.GetAddressOf()));

    HR(CreateShaderFromFile(
        (dir_path_ / "HLSL" / "Triangle_PS.cso").wstring(),
        path_manager_.find_path("HLSL\\Triangle_PS.hlsl").wstring(), "PS",
        "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pd3dDevice_->CreatePixelShader(blob->GetBufferPointer(),
                                      blob->GetBufferSize(), nullptr,
                                      pPixelShader_.GetAddressOf()));

    return true;
  }
  bool InitResource() {
    using namespace DirectX;
    // this->shapes_.emplace_back(std::make_unique<CubeMesh>());
    // auto mesh = dynamic_cast<CubeMesh *>(this->shapes_[0].get());
    cube_ = std::make_unique<CubeMesh>(pd3dDevice_.Get(),
                                       pd3dDeviceIMContext_.Get());
    auto mesh = cube_.get();

    Model *model = new Model(pd3dDevice_.Get(), pd3dDeviceIMContext_.Get(),
                             path_manager_.find_path(L"bunny.obj"));
    models_.push_back(std::unique_ptr<Model>(model));
    HRESULT hr;
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
    CBuffer_.View = DirectX::XMMatrixTranspose(CBuffer_.View);

    // 设置图元类型，设定输入布局
    pd3dDeviceIMContext_->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dDeviceIMContext_->IASetInputLayout(pVertexLayout_.Get());
    // 将着色器绑定到渲染管线
    pd3dDeviceIMContext_->VSSetShader(pVertexShader_.Get(), nullptr, 0);
    pd3dDeviceIMContext_->VSSetConstantBuffers(0, 1, pCBuffer_.GetAddressOf());
    pd3dDeviceIMContext_->PSSetShader(pPixelShader_.Get(), nullptr, 0);

    D3D11SetDebugObjectName(pVertexLayout_.Get(), "VertexPosColorLayout");
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