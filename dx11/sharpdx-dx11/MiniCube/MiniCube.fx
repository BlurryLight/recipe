struct VS_IN
{
	float4 pos : POSITION;
	float4 col : COLOR;
};

struct PS_IN
{
	float4 pos : SV_POSITION;
	float4 ndc_coords : TEXCOORD0;
	float4 col : COLOR;
};

float4x4 worldViewProj;

PS_IN VS( VS_IN input )
{
	PS_IN output = (PS_IN)0;
	
	output.pos = mul(input.pos, worldViewProj);
	output.ndc_coords = output.pos;
	output.col = input.col;
	
	return output;
}

float4 PS(PS_IN input) : SV_Target
{
	//return input.col;
	float3 ndc = input.ndc_coords.xyz / input.ndc_coords.w;
	return float4(ndc.x * 0.5 + 0.5,ndc.y * 0.5 + 0.5,ndc.z,1.0);
//	return float4(1.0,1.0,0.0,1.0);
}