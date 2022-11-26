cbuffer cbPerObject : register(b0) {
  float4x4 gWorldViewProj;
  float gTime;
};

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

  vin.PosL.z *= 0.6f + 0.4f * sin(2.0 * gTime);

  vout.PosH = mul(vin.PosL, gWorldViewProj);
  return vout;
};

float4 PS(VertexOut vout) : SV_Target {
  return float4(vout.Color.xyz *
                    max(float3(sin(gTime), cos(gTime), sin(gTime)), 0.5),
                1.0);
}