struct PSInput
{
    float4 position : SV_POSITION;
    float2 texPos : TEXCOORD;
    uint16_t Test : TESTVAL;
    float3 normal : NORMAL;
    uint TextureIndex : TEXIND;
};
Texture2D simpleTexture : register(t0);
SamplerState ssampler : register(s0);

float4 PSmain(PSInput input) : SV_TARGET
{
    if (input.Test == 0) {
        return simpleTexture.Sample(ssampler, input.texPos);
    }
    else if (input.Test == 1) {
        return float4(abs(input.normal.x), abs(input.normal.y), abs(input.normal.z), 0);
    }else if (input.Test == 2){
        return { 0.5, 0.5, 0, 0 };
    } else if (input.Test == 3) {
        return { 0, 1, 0, 0 };
    } else if (input.Test == 4) {
        return { 0, 0.5, 0.5, 0 };
    } else if (input.Test == 5) {
        return { 0.25, 0.25, 0.5, 0 };
    } else if (input.Test == 6) {
        return { 0.25, 0.5, 0.25, 0 };
    } else if (input.Test == 7) {
        return { 1, 0, 0, 0 };
    } else {
        return { 0.33, 0.33, 0.33, 0 };
    }
}