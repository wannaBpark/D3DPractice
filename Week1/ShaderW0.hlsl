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

// SV_POSITION : 래스터화기 단계로 입력되는 위 시맨틱은 반드시 투영변환의 결과여야 하며, clipping space 기준으로 한 위치여야 한다
struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader 
    float4 color : COLOR;          // Color to pass to the pixel shader
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // 상수 버퍼를 통해 넘겨 받은 Offset을 더해 버텍스를 이동
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