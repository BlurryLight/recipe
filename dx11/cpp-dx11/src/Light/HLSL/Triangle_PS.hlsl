#include "Triangle.hlsli"

Texture2D my_texture;
SamplerState my_sampler;

float4 BlinnPhongShading(DirectionalLight light, PhongMaterial mat,
                         VertexOut fragment) {
  float4 ambient = mat.ambient * light.color;

  float3 lightDir = normalize(-light.dir.xyz);
  float3 viewDir = normalize(g_CamPosW - fragment.posW);
  float3 halfway = normalize(viewDir + lightDir);

  float4 diffuse = saturate(dot(fragment.normalW, lightDir)) * light.color;
  float4 specular =
      pow(saturate(dot(fragment.normalW, halfway)), mat.specular.w) *
      light.color;
  float4 res = (ambient + diffuse) * mat.diffuse + specular * mat.specular;
  return res;
}
// 像素着色器
float4 PS(VertexOut pIn) : SV_Target {
  // renormalize  normal
  pIn.normalW = normalize(pIn.normalW);
  return float4(BlinnPhongShading(g_DirLight, g_Mat, pIn).rgb *
                    my_texture.Sample(my_sampler, pIn.uv).rgb,
                1.0);
  // return float4(pIn.normalW.rgb * 0.5 + 0.3, 1.0);
}
