#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;

uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

// renderMode:
//   0 = solid (Blinn-Phong lit)
//   1 = lines / wireframe — full objectColor, no lighting applied
//   2 = edge darkening overlay (for SolidWithEdges mode)
uniform int renderMode;

// Selection highlight
uniform int isSelected;

void main() {
    vec3 baseColor = objectColor;

    // Selection highlight: subtle blue tint
    if (isSelected == 1) {
        baseColor = mix(baseColor, vec3(0.3, 0.5, 0.9), 0.25);
    }

    // ── Line / wireframe mode: use color directly ──────────────────────────────
    // No lighting — axes, sketch lines, preview bands all use full objectColor.
    if (renderMode == 1) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    // ── Edge darkening overlay (renderMode == 2) ───────────────────────────────
    if (renderMode == 2) {
        FragColor = vec4(baseColor * 0.18, 1.0);
        return;
    }

    // ── Solid (renderMode == 0): Blinn-Phong ──────────────────────────────────
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Ambient
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse — soft wrap lighting
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    diff = smoothstep(0.0, 1.0, diff);
    diff = max(diff, 0.15);
    vec3 diffuse = diffuseStrength * diff * lightColor;

    // Specular — Blinn-Phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    spec = smoothstep(0.05, 1.0, spec);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse) * baseColor + specular;

    // Subtle gamma correction
    result = pow(max(result, vec3(0.0)), vec3(0.88));

    FragColor = vec4(result, 1.0);
}
