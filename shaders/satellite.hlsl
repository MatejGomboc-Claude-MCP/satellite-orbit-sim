// Vertex Shader
struct VSInput {
    float3 position : POSITION;
};

struct VSOutput {
    float4 position : SV_POSITION;
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
    
    // Point size is handled via the pipeline's rasterizer state in Vulkan
    // This will be set to a fixed size in the renderer
    
    return output;
}

// Pixel Shader
float4 PSMain(VSOutput input, float2 pointCoord : SV_Position) : SV_TARGET {
    // In DirectX/HLSL, for point sprites we can use the fragment's position
    // to determine the distance from the center

    // Get the current screen pixel position
    float2 fragCoord = pointCoord.xy;
    
    // Get the center of the point sprite
    float2 center = fragCoord;
    
    // Calculate distance from center (normalized to 0-1)
    float dist = length(center) / 10.0; // Scale based on point size
    
    // Create a circular point with soft edges
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    
    // If outside the circle, discard the fragment
    if (alpha <= 0.0) {
        discard;
    }
    
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
