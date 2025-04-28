// Vertex Shader
struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

cbuffer UniformBufferObject : register(b0) {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    
    // Earth color (blue with some green)
    output.color = float3(0.0, 0.3, 0.8);
    
    // Transform position to world space
    float4 worldPosition = mul(model, float4(input.position, 1.0));
    
    // Pass world position to fragment shader
    output.worldPos = worldPosition.xyz;
    
    // Transform normal to world space
    output.normal = mul((float3x3)model, input.normal);
    
    // Project to screen coordinates
    output.position = mul(proj, mul(view, worldPosition));
    
    return output;
}

// Pixel Shader
float4 PSMain(VSOutput input) : SV_TARGET {
    // Use a simple lighting model
    
    // Define light direction (from the center outward)
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    
    // Normalize the fragment normal
    float3 normal = normalize(input.normal);
    
    // Calculate diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Add ambient lighting
    float ambient = 0.2;
    
    // Calculate the final lighting
    float lighting = ambient + diff * 0.8;
    
    // Apply lighting to the color
    float4 color = float4(input.color * lighting, 1.0);
    
    // Add a slight atmospheric glow at the edges
    float rim = 1.0 - max(dot(normalize(-input.worldPos), normal), 0.0);
    rim = pow(rim, 4.0);
    
    // Mix in the rim light (subtle blue glow)
    color.rgb = lerp(color.rgb, float3(0.3, 0.7, 1.0), rim * 0.3);
    
    return color;
}
