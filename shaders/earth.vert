#version 450

// Input attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

// Output to the fragment shader
layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;

// Uniform buffer with transformation matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    // Earth color (blue with some green)
    fragColor = vec3(0.0, 0.3, 0.8);
    
    // Transform position to world space
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);
    
    // Pass world position to fragment shader
    fragPosition = worldPosition.xyz;
    
    // Transform normal to world space
    fragNormal = mat3(ubo.model) * inNormal;
    
    // Project to screen coordinates
    gl_Position = ubo.proj * ubo.view * worldPosition;
}
