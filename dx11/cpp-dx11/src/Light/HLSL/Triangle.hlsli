cbuffer ConstantBuffer : register(b0) {
  matrix g_World;
  matrix g_WorldInverseTranspose;
  matrix g_View;
  matrix g_Proj;
}

struct VertexIn {
  float3 pos : POSITION;
  float3 normal : NORMAL;
  float2 tex : TEXCOORD;
};

struct VertexOut {
  float4 posH : SV_POSITION;
  float4 normal : TEXCOORD0;
};
