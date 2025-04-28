#version 450

// Input from vertex shader
layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Use a simple lighting model
    
    // Define light direction (from the center outward)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    
    // Normalize the fragment normal
    vec3 normal = normalize(fragNormal);
    
    // Calculate diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Add ambient lighting
    float ambient = 0.2;
    
    // Calculate the final lighting
    float lighting = ambient + diff * 0.8;
    
    // Apply lighting to the color
    outColor = vec4(fragColor * lighting, 1.0);
    
    // Add a slight atmospheric glow at the edges
    float rim = 1.0 - max(dot(normalize(-fragPosition), normal), 0.0);
    rim = pow(rim, 4.0);
    
    // Mix in the rim light (subtle blue glow)
    outColor.rgb = mix(outColor.rgb, vec3(0.3, 0.7, 1.0), rim * 0.3);
}
