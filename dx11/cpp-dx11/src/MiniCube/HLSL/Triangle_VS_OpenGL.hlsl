#include "Triangle.hlsli"

// 顶点着色器
VertexOut VS(VertexIn vIn) {
  VertexOut vOut;
  float4 posL = float4(vIn.pos, 1.0f);
  float4 posW = mul(g_World, posL);
  float4 posV = mul(g_View, posW);
  vOut.posH = mul(g_Proj, posV);
  vOut.color = vIn.color; // 这里alpha通道的值默认为1.0
  return vOut;
}
