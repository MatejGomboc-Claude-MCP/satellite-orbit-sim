#version 450

// Input positions
layout(location = 0) in vec3 inPosition;

// Uniform buffer with transformation matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    // Transform the satellite position to clip space
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    // Set the point size (satellite will be rendered as a point)
    gl_PointSize = 10.0;
}
