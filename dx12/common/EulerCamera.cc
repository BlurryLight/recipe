#include "EulerCamera.hh"
#include <algorithm>
using namespace PD;

using namespace DirectX::SimpleMath;
using namespace PD;

void Camera::updateCameraVectors() {
  float3 front{};
  front.x = cos(DirectX::XMConvertToRadians(Pitch)) *
            cos(DirectX::XMConvertToRadians(Yaw));
  front.y = sin(DirectX::XMConvertToRadians(Pitch));
  front.z = cos(DirectX::XMConvertToRadians(Pitch)) *
            sin(DirectX::XMConvertToRadians(Yaw));
  front.Normalize(this->Front);
  // 注意DX的Cross是左手表示，注意正负号
  WorldUp.Cross(Front).Normalize(Right);
  Front.Cross(Right).Normalize(Up);
}
Camera::Camera(float3 position, float3 front, float3 up)
    : Position(position), Front(front), WorldUp(up), Yaw(kYAW), Pitch(kPITCH),
      MovementSpeed(kSPEED), MouseSensitivity(kSENSITIVITY), Zoom(kZOOM) {
  updateCameraVectors();
}

DirectX::XMMATRIX Camera::GetViewMatrix() {
  return DirectX::XMMatrixLookToLH(Position, Front, Up);
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime) {
  float velocity = deltaTime * MovementSpeed;
  switch (direction) {
  case CameraMovement::FORWARD:
    Position = Position + velocity * Front;
    break;
  case CameraMovement::BACKWARD:
    Position = Position - velocity * Front;
    break;
  case CameraMovement::LEFT:
    Position = Position - velocity * Right;
    break;
  case CameraMovement::RIGHT:
    Position = Position + velocity * Right;
    break;
  }
}
void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
  xoffset *= MouseSensitivity;
  yoffset *= MouseSensitivity;

  Yaw += xoffset;
  Pitch += yoffset;

  // Make sure that when pitch is out of bounds, screen doesn't get flipped
  Pitch = std::clamp(Pitch, -89.99f, 89.99f);
  // Update Front, Right and Up Vectors using the updated Euler angles
  updateCameraVectors();
}
