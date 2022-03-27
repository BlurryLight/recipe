#ifndef GAMEAPP_H
#define GAMEAPP_H
#include <d3dApp.hh>
#include <d3dUtils.hh>
#include <imgui/imgui.h>
namespace PD {
class GameApp : public D3DApp {
public:
  GameApp() : D3DApp(){};
  ~GameApp(){};

  void UpdateScene(float dt) override{};
  void DrawImGUI() override {
    D3DApp::DrawImGUI();
    if (show_demo_window_) {
      ImGui::ShowDemoWindow(&show_demo_window_);
    }
    {
      ImGui::Begin("another window");
      ImGui::Checkbox("Demo", &show_demo_window_);
      ImGui::End();
    }
  }
  void DrawScene() override {
    assert(pd3dDeviceIMContext_);
    assert(pSwapChain_);
    static float blue[4] = {0.0f, 0.0f, 1.0f, 1.0f}; // RGBA = (0,0,255,255)
    pd3dDeviceIMContext_->ClearRenderTargetView(pRenderTargetView_.Get(), blue);
    pd3dDeviceIMContext_->ClearDepthStencilView(
        pDepthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f,
        0);
  };

private:
  bool show_demo_window_ = true;
};
} // namespace PD
int main() {
  PD::GameApp app;
  app.Init();
  app.Run();
  return 0;
}

#endif