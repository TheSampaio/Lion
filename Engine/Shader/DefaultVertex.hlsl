cbuffer ConstantBuffer
{
    float4x4 WorldViewProj;
}

struct VertexIn
{
    float3 Pos : POSITION;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD;
};

struct VertexOut
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD;
};

VertexOut main(VertexIn vIn)
{
    VertexOut vOut;

    vOut.Pos = mul(float4(vIn.Pos, 1.0f), WorldViewProj);
    
    vOut.Color = vIn.Color;

    vOut.Tex = vIn.Tex;

    return vOut;
}
