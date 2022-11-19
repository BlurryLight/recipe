cbuffer cbPerObject : register(b0) { float4x4 gWorldViewProj; };

struct VertexIn {
  float4 PosL : POSITION;
  float4 Color : COLOR;
};

struct VertexOut {
  float4 PosH : SV_Position;
  float4 Color : COLOR;
};

VertexOut VS(VertexIn vin) {
  VertexOut vout;
  vout.Color = vin.Color;
  vout.PosH = mul(vin.PosL, gWorldViewProj);
  return vout;
};

float4 PS(VertexOut vout) : SV_Target { return vout.Color; }