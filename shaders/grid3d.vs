#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

out vec3 nearPoint;
out vec3 farPoint;

// Unproject grid vertices to world space
vec3 UnprojectPoint(float x, float y, float z, mat4 viewInv, mat4 projInv) {
    vec4 unprojected = viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojected.xyz / unprojected.w;
}

void main() {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    
    nearPoint = UnprojectPoint(aPos.x, aPos.y, 0.0, viewInv, projInv);
    farPoint = UnprojectPoint(aPos.x, aPos.y, 1.0, viewInv, projInv);
    
    gl_Position = vec4(aPos, 1.0);
}
