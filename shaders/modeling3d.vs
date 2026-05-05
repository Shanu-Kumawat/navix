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
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    
    // Clip plane support (for cross-sections)
    gl_ClipDistance[0] = dot(vec4(FragPos, 1.0), clipPlane);
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
