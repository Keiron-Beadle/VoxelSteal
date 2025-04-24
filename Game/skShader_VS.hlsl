
cbuffer ConstantBuffer : register(b0) {
	float4x4 mvp;
	float3 col;
}

struct VS_IN {
	float4 pos : POSITION;
	float3 col : COLOUR;
};

struct VS_OUT {
	float4 pos : SV_POSITION;
	float3 col : COLOUR;
};

VS_OUT main( VS_IN input, uint id : SV_VertexID )
{
	VS_OUT output = (VS_OUT)0;
	
	output.pos = mul(mvp, input.pos);
	output.col = float4(col,1);
	//output.pos = input.pos;
	
	//output.col = float3(1, 0, 1);
	return output;
}