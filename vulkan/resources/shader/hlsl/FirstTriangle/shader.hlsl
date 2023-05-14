struct VertexOut {
  float4 posH : SV_POSITION;
  float4 color : COLOR;
};

struct VertexIn {
  [[vk::location(0)]] float2 pos : POSITION;
  [[vk::location(1)]] float3 color : COLOR;
};

// const static float2 positions[3] = {float2(0.0, -0.5), float2(0.5, 0.5),
//                                     float2(-0.5, 0.5)};

// const static float3 colors[3] = {float3(0.0, 1.0, 0.0), float3(0.0, 1.0,
// 0.0),
//                                  float3(1.0, 0.0, 1.0)};

VertexOut VSMain(int VertexID : SV_VertexID, VertexIn vin) {
  VertexOut vOut;
  // vOut.posH = float4(positions[VertexID], 1.0f, 1.0f);
  // vOut.color = float4(colors[VertexID], 1.0f);

  vOut.posH = float4(vin.pos, 1.0f, 1.0f);
  vOut.color = float4(vin.color, 1.0f);
  return vOut;
}

float4 PSMain(VertexOut pIn) : SV_Target0 { return pIn.color; }