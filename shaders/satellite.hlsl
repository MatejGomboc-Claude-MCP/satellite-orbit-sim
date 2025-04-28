// Vertex Shader
struct VSInput {
    float3 position : POSITION;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float pointSize : PSIZE;
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
    
    // Set the point size for the satellite
    output.pointSize = 10.0;
    
    return output;
}

// Pixel Shader
float4 PSMain(VSOutput input, float2 pointCoord : SV_Position) : SV_TARGET {
    // In HLSL for Vulkan via SPIR-V, we can access the point coordinates
    // via a system-generated value during rasterization
    
    // Calculate center-relative coordinates
    float2 uv = pointCoord.xy; 
    
    // Calculate distance from center (using normalized device coordinates)
    float2 center = float2(0.5, 0.5);
    float dist = length(uv - center) * 2.0;  
    
    // Create a circular point with soft edges
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    
    // Discard pixels outside the circle
    if (alpha < 0.01)
        discard;
    
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
