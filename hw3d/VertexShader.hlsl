struct VSInput
{
    float3 position : POSITION;
    float2 texPos : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texPos : TEXCOORD;
};

cbuffer cbuffer0 : register(b0)
{
    float4x4 viewMatrix;
};

PSInput main(VSInput input)
{
    PSInput result;
    
    result.position = mul(float4(input.position, 1.0f), viewMatrix);
    result.texPos = input.texPos;
    
    return result;
}
