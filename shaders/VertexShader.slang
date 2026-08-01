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
    //This has 12Bytes of padding because float4x4 in hlsl is 16 byte aligned while in c++ its 4 byte aligned
};

cbuffer cbuffer0 : register(b0)
{
    CBVData cbvData[819];
};

PSInput VSmain(VSInput input)
{
    PSInput result;
    
    result.position = mul(float4(input.position, 1.0), cbvData[input.Instance].viewMatrix);
    result.texPos = input.texPos;
    
    return result;
}