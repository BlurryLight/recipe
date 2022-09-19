#include "Triangle.hlsli"

// 顶点着色器
VertexOut VS(VertexIn vIn) {
  VertexOut vOut;
  float4 posL = float4(vIn.pos, 1.0f);
  float4 posW = mul(posL, g_World);
  float4 posV = mul(posW, g_View);
  vOut.posH = mul(posV, g_Proj);
  vOut.normal = mul(float4(vIn.normal, 1.0), g_WorldInverseTranspose);
  return vOut;
}
