cbuffer ConstantBuffer : register(b0) {
	float4x4 mvp;
	float3 col;
}

struct VS_OUT {
	float4 pos : SV_POSITION;
	float3 col : COLOUR;
};


float4 main(VS_OUT input) : SV_TARGET
{
	return float4(input.col,1.0);
}