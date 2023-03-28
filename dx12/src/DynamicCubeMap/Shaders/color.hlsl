#include "LightingUtils.hlsl"
#include "common.hlsl"

struct VertexIn {
  float3 PosL : POSITION;
  float3 Normal : NORMAL;
  float2 TexC : TEXCOORD;
};

struct VertexOut {
  float4 PosH : SV_Position;
  float3 PosW : Position;
  float3 NormalW : NORMAL;
  float2 TexC : TEXCOORD;
};

VertexOut VSMain(VertexIn vin) {
  VertexOut vout = (VertexOut)0.0f;

  vout.NormalW = mul(vin.Normal, (float3x3)(gInvTransWorld));

  float4 PosW = mul(float4(vin.PosL, 1.0), gWorld);
  vout.PosW = PosW.xyz;
  vout.PosH = mul(PosW, gViewProj);

  MaterialData matData = gMaterialData[gMaterialIndex];
  float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
  vout.TexC = mul(texC, matData.gMatTransform).xy;

  return vout;
};

float4 PSMain(VertexOut vout) : SV_Target {

  vout.NormalW = normalize(vout.NormalW); // re-normalize
  // return float4(0.5 * vout.NormalW + 0.5, 1);
  float3 viewDir = normalize(gEyePosW - vout.PosW);
  MaterialData matData = gMaterialData[gMaterialIndex];
  float3 ambient = gAmbientLight * gDiffuseMap[matData.DiffuseMapIndex].Sample(
                                       gsamLinearWrap, vout.TexC);
  Material mat;
  mat.DiffuseAlbedo = matData.gDiffuseAlbedo;
  mat.FresnelR0 = matData.gFresnelR0;
  mat.Shininess = max(1 - matData.gRoughness, 0.001);
  float4 litColor =
      float4(ambient + ComputeDirLight(gLights[0], mat, vout.NormalW, viewDir) +
                 ComputeDirLight(gLights[1], mat, vout.NormalW, viewDir) +
                 ComputeDirLight(gLights[2], mat, vout.NormalW, viewDir),
             1.0);
  float3 r = reflect(-viewDir, vout.NormalW);
  float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
  float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, vout.NormalW, r);
  litColor.rgb += mat.Shininess * fresnelFactor * reflectionColor.rgb;

  litColor.a = mat.DiffuseAlbedo.a;
  return litColor;
}