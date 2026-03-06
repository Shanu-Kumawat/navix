#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormalOrColor;

out vec3 FragPos;
out vec3 Normal;
out vec3 VertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 clipPlane;
uniform int useVertexColor;

void main() {
    // Calculate fragment position in world space
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    if (useVertexColor == 1) {
        // When rendering stress contours, attribute 1 is vertex color
        Normal = vec3(0.0, 1.0, 0.0); // dummy normal; recompute in FS
        VertexColor = aNormalOrColor;
    } else {
        // Normal rendering: attribute 1 is the surface normal
        Normal = normalize(mat3(transpose(inverse(model))) * aNormalOrColor);
        VertexColor = vec3(0.0);
    }
    
    // Apply clip plane if needed
    gl_ClipDistance[0] = dot(vec4(FragPos, 1.0), clipPlane);
    
    // Calculate final position
    gl_Position = projection * view * vec4(FragPos, 1.0);
} 