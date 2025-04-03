#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 clipPlane;

void main() {
    // Calculate fragment position in world space
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Calculate normal vector - use proper normal transformation
    // This ensures more accurate normals for better lighting and edge detection
    Normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    
    // Apply clip plane if needed
    gl_ClipDistance[0] = dot(vec4(FragPos, 1.0), clipPlane);
    
    // Calculate final position
    gl_Position = projection * view * vec4(FragPos, 1.0);
} 