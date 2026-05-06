#version 330 core

in vec2 vUV;
out vec4 FragColor;

void main() {
    // Match UIColors::GRID_BACKGROUND = (0.078, 0.086, 0.125) = #141620
    // Slightly lighter at horizon for depth, darker at top and bottom corners
    vec3 midColor = vec3(0.078f, 0.086f, 0.125f);   // exact 2D canvas BG
    vec3 topColor = vec3(0.055f, 0.060f, 0.090f);   // slightly deeper at top
    vec3 botColor = vec3(0.090f, 0.098f, 0.138f);   // slightly warmer at base

    float t = vUV.y;
    vec3 bg;
    if (t > 0.5)
        bg = mix(midColor, topColor, (t - 0.5) * 2.0);
    else
        bg = mix(botColor, midColor, t * 2.0);

    // Subtle vignette — corners darker, focus center
    vec2 uv2 = vUV * 2.0 - 1.0;
    float r2 = dot(uv2, uv2);
    float vignette = 1.0 - 0.22 * r2 * r2;
    bg *= clamp(vignette, 0.0, 1.0);

    FragColor = vec4(bg, 1.0);
}
