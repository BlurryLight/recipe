cbuffer VSConstantBuffer : register(b1) {
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
  float3 normalW : TEXCOORD0;
  float3 posW : TEXCOORD1;
};

struct DirectionalLight {
  float4 color;
  float4 dir;
};
struct PhongMaterial {
  float4 ambient;
  float4 diffuse;
  float4 specular; // w is power
};
cbuffer PSConstantBuffer : register(b0) {
  DirectionalLight g_DirLight;
  PhongMaterial g_Mat;
  float3 g_CamPosW;
  float pading;
};