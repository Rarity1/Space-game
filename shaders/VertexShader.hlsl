struct VSInput
{
    float3 normal : NORMAL;
    float3 position : POSITION;
    float2 texPos : TEXCOORD;
    uint Instance : SV_InstanceID;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texPos : TEXCOORD;
};

struct CBVData
{
    float4x4 viewMatrix;
    uint texIdex;
};

cbuffer cbuffer0 : register(b0)
{
    CBVData cbvData;
};

PSInput main(VSInput input)
{
    PSInput result;
    
    result.position = mul(float4(input.position, 1.0), cbvData.viewMatrix);
    result.texPos = input.texPos;
    
    return result;
}
