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

// Render mode: 0 = solid, 1 = wireframe overlay, 2 = edge-only
uniform int renderMode;

// Selection highlight
uniform int isSelected;

void main() {
    vec3 baseColor = objectColor;

    // Selection highlight: subtle blue tint
    if (isSelected == 1) {
        baseColor = mix(baseColor, vec3(0.3, 0.5, 0.9), 0.25);
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Ambient
    vec3 ambient = ambientStrength * lightColor * 1.15;

    // Diffuse — Blinn-Phong with soft wrap
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    diff = smoothstep(0.0, 1.0, diff);
    diff = max(diff, 0.15); // Minimum diffuse to reduce harsh shadows
    vec3 diffuse = diffuseStrength * diff * lightColor;

    // Specular — Blinn-Phong
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    spec = smoothstep(0.05, 1.0, spec);
    vec3 specular = specularStrength * spec * lightColor;

    // Metallic tint
    vec3 metalColor = baseColor * vec3(1.03, 1.03, 1.05);

    // Combined
    vec3 result = (ambient + diffuse) * metalColor + specular;

    // Subtle gamma correction for brightness
    result = pow(result, vec3(0.88));

    // Edge darkening for wireframe mode
    if (renderMode == 1) {
        result = baseColor * 0.15;
    }

    FragColor = vec4(result, 1.0);
}
