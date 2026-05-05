#version 330 core

in vec3 nearPoint;
in vec3 farPoint;

out vec4 FragColor;

uniform mat4 view;
uniform mat4 projection;
uniform float gridScale;

float computeDepth(vec3 pos) {
    vec4 clipPos = projection * view * vec4(pos, 1.0);
    return (clipPos.z / clipPos.w) * 0.5 + 0.5;
}

vec4 grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);

    // Subtle thin grid lines
    float alpha = 1.0 - min(line, 1.0);
    vec4 color = vec4(0.30, 0.32, 0.38, alpha * 0.35);  // Very subtle gray

    return color;
}

void main() {
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    if (t < 0.0 || t > 1.0) discard;

    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);
    gl_FragDepth = computeDepth(fragPos3D);

    // Fine grid + coarse grid
    float scale1 = gridScale;
    float scale2 = gridScale * 0.1;

    vec4 fineGrid  = grid(fragPos3D, scale1);
    vec4 coarseGrid = grid(fragPos3D, scale2);

    // Coarse grid is slightly more visible
    coarseGrid.a *= 1.8;

    // Blend: coarse on top of fine
    FragColor = fineGrid;
    FragColor.rgb = mix(FragColor.rgb, coarseGrid.rgb, coarseGrid.a);
    FragColor.a = max(FragColor.a, coarseGrid.a * 0.6);

    // === Origin axis lines (prominent, colored) ===
    float axisWidth = 2.0;  // px-ish width

    // X axis (red line along Z=0)
    float zDist = abs(fragPos3D.z) * scale2;
    float zLine = clamp(1.0 - zDist / (fwidth(fragPos3D.z * scale2) * axisWidth), 0.0, 1.0);
    if (zLine > 0.01) {
        FragColor.rgb = mix(FragColor.rgb, vec3(0.85, 0.22, 0.22), zLine * 0.85);
        FragColor.a = max(FragColor.a, zLine * 0.9);
    }

    // Z axis (blue line along X=0)
    float xDist = abs(fragPos3D.x) * scale2;
    float xLine = clamp(1.0 - xDist / (fwidth(fragPos3D.x * scale2) * axisWidth), 0.0, 1.0);
    if (xLine > 0.01) {
        FragColor.rgb = mix(FragColor.rgb, vec3(0.22, 0.40, 0.85), xLine * 0.85);
        FragColor.a = max(FragColor.a, xLine * 0.9);
    }

    // Distance-based fade
    float dist = length(fragPos3D.xz);
    float fadeStart = 40.0 / gridScale;
    float fadeEnd   = 120.0 / gridScale;
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
    FragColor.a *= fade;

    if (FragColor.a < 0.005) discard;
}
