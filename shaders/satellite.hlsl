// Vertex Shader
struct VSInput {
    float3 position : POSITION;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 pointCoord : TEXCOORD0;  // We'll need to calculate this in pixel shader
};

cbuffer UniformBufferObject : register(b0) {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    
    // Transform the satellite position to clip space
    output.position = mul(proj, mul(view, mul(model, float4(input.position, 1.0))));
    
    // In HLSL, we need to handle point size differently from GLSL
    // We'll use a constant here since Vulkan will control the point size
    // and we'll calculate point coordinates in the pixel shader
    output.pointCoord = float2(0.0, 0.0); // This will be replaced in the PS
    
    return output;
}

// Pixel Shader
float4 PSMain(VSOutput input, float2 pointCoord : SV_Position) : SV_TARGET {
    // In Direct3D/HLSL, we need to calculate the point coordinate differently
    // Here, we can use the built-in SV_Position with some adjustments
    
    // Calculate distance from center of point
    float2 center = pointCoord - float2(0.5, 0.5);
    float dist = length(center);
    
    // Create a circular point with soft edges
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    
    // Set the satellite color (bright white/yellow)
    float4 color = float4(1.0, 0.9, 0.5, alpha);
    
    // Add a glow effect
    if (dist > 0.2) {
        // Outer glow (yellow/orange)
        float3 glowColor = float3(1.0, 0.6, 0.2);
        float glowIntensity = smoothstep(0.5, 0.2, dist);
        
        // Mix the glow color with the base color
        color.rgb = lerp(glowColor, color.rgb, glowIntensity);
    }
    
    return color;
}
