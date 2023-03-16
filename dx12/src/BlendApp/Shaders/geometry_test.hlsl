cbuffer cbPerObject : register(b0) {
  float4x4 gWorld;
  float4x4 gInvTransWorld;
  float4x4 gTexTransform;
};

Texture2DArray gTreeMapArray : register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

struct VertexIn {
  float3 PosW : POSITION;
  float2 SizeW : SIZE;
};

struct VertexOut {

  float3 CenterW : POSITION;
  float2 SizeW : SIZE;
};

struct GeoOut {

  float4 PosH : SV_Position;
  float3 PosW : POSITION;
  float3 NormalW : NORMAL;
  float2 TexC : TEXCOORD;
  uint PrimID : SV_PrimitiveID;
};

cbuffer cbPass : register(b1) {
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProj;
  float4x4 gInvProj;
  float4x4 gViewProj;
  float4x4 gInvViewProj;
  float3 gEyePosW;
  float cbPerObjectPad1;
  float2 gRenderTargetSize;
  float2 gInvRenderTargetSize;
  float gNearZ;
  float gFarZ;
  float gTotalTime;
  float gDeltaTime;
};

VertexOut VSMain(VertexIn vin) {
  VertexOut vout = (VertexOut)(0);
  vout.CenterW = vin.PosW;
  vout.SizeW = vin.SizeW;
  return vout;
};

[maxvertexcount(4)] void GSMain(point VertexOut gin[1], uint primID
                                : SV_PrimitiveID,
                                  inout TriangleStream<GeoOut> tris) {
  float3 up = float3(0.0f, 1.0f, 0.0f);
  float3 look = gEyePosW - gin[0].CenterW;
  look.y = 0.0f; // y-axis aligned, so project to xz-plane
  look = normalize(look);
  float3 right = cross(up, look);

  //
  // Compute triangle strip vertices (quad) in world space.
  //
  float halfWidth = 0.5f * gin[0].SizeW.x;
  float halfHeight = 0.5f * gin[0].SizeW.y;

  float4 v[4];
  v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0f);
  v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0f);
  v[2] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0f);
  v[3] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0f);

  float2 texC[4] = {float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f),
                    float2(1.0f, 0.0f)};

  GeoOut gout;
  [unroll] for (int i = 0; i < 4; ++i) {
    gout.PosH = mul(v[i], gViewProj);
    gout.PosW = v[i].xyz;
    gout.NormalW = look;
    gout.TexC = texC[i];
    gout.PrimID = primID;

    tris.Append(gout);
  }
}

float4 PSMain(GeoOut gin)
    : SV_Target {

  float3 uvw = float3(gin.TexC, gin.PrimID % 3);
  float4 color = gTreeMapArray.Sample(gsamLinearClamp, uvw);
  clip(color.a - 0.1);
  return color;
}