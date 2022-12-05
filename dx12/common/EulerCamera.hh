
#pragma once
// take form
// https://github.com/BlurryLight/DiRenderLab/blob/main/GLwrapper/glsupport.hh

#include "d3dApp.hh"
#include "SimpleMath.h"

namespace PD {
constexpr inline float kYAW = 90.0f;
constexpr inline float kPITCH = 0.0f;
constexpr inline float kSPEED = 2.5f;
constexpr inline float kSENSITIVITY = 0.1f;
constexpr inline float kZOOM = 45.0f;
enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT };
class Camera {
  using float3 = DirectX::SimpleMath::Vector3;

public:
  float3 Position{};
  float3 Front{};
  float3 WorldUp{};
  float3 Right{};
  // Euler Angles
  float Yaw;
  float Pitch;
  // Camera options
  float MovementSpeed;
  float MouseSensitivity;
  float Zoom;
  float3 Up{};
  Camera(float3 position = float3(0.0f, 0.0f, 0.0f),
         float3 front = float3(0.0f, 0.0f, 1.0f),
         float3 up = float3(0.0f, 1.0f, 0.0f));
 // without transpotation
  DirectX::XMMATRIX GetViewMatrix();
  void ProcessKeyboard(CameraMovement direction, float deltaTime);

  void ProcessMouseMovement(float xoffset, float yoffset);
  //   void ProcessMouseScroll(float mouseOffset);

private:
  void updateCameraVectors();
};
} // namespace PD