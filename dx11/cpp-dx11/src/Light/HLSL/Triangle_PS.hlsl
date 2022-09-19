#include "Triangle.hlsli"

// 像素着色器
float4 PS(VertexOut pIn) : SV_Target {
  //   return float4(1, 1, 1, 1);
  return float4(pIn.normal.rgb * 0.5 + 0.5, 1.0);
}
