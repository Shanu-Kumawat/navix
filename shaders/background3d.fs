#version 330 core

in vec2 vUV;
out vec4 FragColor;

void main() {
    // Studio-dark gradient: deep navy-charcoal top, slightly warmer base
    vec3 topColor    = vec3(0.072, 0.078, 0.098);   // Very dark blue-slate #121420
    vec3 midColor    = vec3(0.098, 0.103, 0.128);   // #191a21
    vec3 botColor    = vec3(0.132, 0.138, 0.162);   // #222329

    // Two-section gradient for more depth
    float t = vUV.y;
    vec3 bg;
    if (t > 0.5) {
        bg = mix(midColor, topColor, (t - 0.5) * 2.0);
    } else {
        bg = mix(botColor, midColor, t * 2.0);
    }

    // Radial vignette — darkens corners, focuses center
    vec2 uv2 = vUV * 2.0 - 1.0;
    float r2 = dot(uv2, uv2);
    float vignette = 1.0 - 0.28 * r2 * r2;
    bg *= clamp(vignette, 0.0, 1.0);

    // Very faint horizontal centerline glow (subtle depth cue)
    float horizGlow = exp(-40.0 * pow(vUV.y - 0.42, 2.0)) * 0.018;
    bg += vec3(horizGlow * 0.6, horizGlow * 0.7, horizGlow);

    FragColor = vec4(bg, 1.0);
}
