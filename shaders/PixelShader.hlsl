struct PSInput
{
    float4 position : SV_POSITION;
    float2 texPos : TEXCOORD;
};
Texture2D simpleTexture : register(t0);
SamplerState ssampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{

   return simpleTexture.Sample(ssampler, input.texPos);

}
