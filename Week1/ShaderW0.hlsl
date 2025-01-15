// ShaderW0.hlsl
cbuffer constants : register(b0)
{
    float3 Offset;
    float Pad;
}

struct VS_INPUT
{
    float4 position : POSITION; // input position from vertex buffer
    float4 color : COLOR;       // input color from vertex buffer
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 color : COLOR;          // Color to pass to the pixel shader
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // ��� ���۸� ���� �Ѱ� ���� Offset�� ���� ���ؽ��� �̵�
    output.position = float4(Offset, 0) + input.position;
    
    // Pass the color to the pixel shader
    output.color = input.color;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}