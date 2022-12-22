#pragma once

#define MaxLights 16
struct Light {
  float3 Strength;
  float FalloffStart; // point/spot
  float3 Direction;   // dir/spot
  float FalloffEnd;
  float3 Position; // point/spot
  float SpotPower; // spot only
};

struct Material {
  float4 DiffuseAlbedo;
  float3 FresnelR0;
  float Shininess;
};

// R0是材质有关的常数，90度直射的材质的反射率
float3 SchlickFresnel(float3 R0, float3 normalW, float3 lightVec) {
  float cosTheta = saturate(dot(lightVec, normalW));
  float tmp = 1.0 - cosTheta;
  float tmp2 = tmp * tmp;

  return R0 + (1 - R0) * (tmp2 * tmp2 * tmp);
}

float3 BlinnPhong(float3 lightStrength, float3 lightDir, float3 normalW,
                  float3 viewDir, Material mat) {
  const float m = mat.Shininess * 256.0f; // 决定高光斑大小
  float3 halfVec = normalize(viewDir + lightDir);

  float roughFactor = (m + 8.0) / 8.0 + pow(max(dot(normalW, halfVec), 0.0), m);
  float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, normalW, lightDir);
  float3 specAlbedo = roughFactor * fresnelFactor;

  // tone mapping
  specAlbedo = specAlbedo / (specAlbedo + 1);
  return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirLight(Light L, Material mat, float3 normalW, float3 viewDir) {
  float3 LightDir = -L.Direction;
  float NdotL = max(dot(LightDir, normalW), 0.0f);
  float3 lightStrength = L.Strength * NdotL;
  return BlinnPhong(lightStrength, LightDir, normalW, viewDir, mat);
}