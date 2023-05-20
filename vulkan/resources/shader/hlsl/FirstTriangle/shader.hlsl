struct VertexOut {
  float4 posH : SV_POSITION;
  float4 color : COLOR;
};

struct VertexIn {
  [[vk::location(0)]] float2 pos : POSITION;
  [[vk::location(1)]] float3 color : COLOR;
};

cbuffer UniformBufferObject : register(b0) {
  float4x4 model;
  float4x4 view;
  float4x4 proj;
};

VertexOut VSMain(int VertexID : SV_VertexID, VertexIn vin) {
  VertexOut vOut;

  vOut.posH = mul(proj, mul(view, mul(model, float4(vin.pos, 0.0f, 1.0f))));
  vOut.color = float4(vin.color, 1.0f);
  return vOut;
}

float4 PSMain(VertexOut pIn) : SV_Target0 { return pIn.color; }