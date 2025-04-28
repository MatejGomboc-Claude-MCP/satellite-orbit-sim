#version 450

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Calculate distance from center of point
    vec2 center = gl_PointCoord - vec2(0.5);
    float dist = length(center);
    
    // Create a circular point with soft edges
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    
    // Set the satellite color (bright white/yellow)
    outColor = vec4(1.0, 0.9, 0.5, alpha);
    
    // Add a glow effect
    if (dist > 0.2) {
        // Outer glow (yellow/orange)
        vec3 glowColor = vec3(1.0, 0.6, 0.2);
        float glowIntensity = smoothstep(0.5, 0.2, dist);
        
        // Mix the glow color with the base color
        outColor.rgb = mix(glowColor, outColor.rgb, glowIntensity);
    }
}
