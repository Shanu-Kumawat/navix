#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 VertexColor;

// Material properties
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;

// Material settings
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

// Render mode (kept for compatibility)
uniform int renderMode;

// Vertex color mode (stress contours)
uniform int useVertexColor;

void main() {
    // Choose base color
    vec3 baseColor = (useVertexColor == 1) ? VertexColor : objectColor;

    // Use pre-normalized normals and protect from zero-length vectors
    vec3 norm;
    if (useVertexColor == 1) {
        // Flat normals from screen-space derivatives
        norm = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
    } else {
        norm = normalize(Normal);
    }
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Increased ambient light to brighten the model overall
    vec3 ambient = ambientStrength * lightColor * 1.2;
    
    // Improved diffuse lighting with smoothing
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // Use smooth diffuse with a minimum value to reduce shadow contrast
    float diffAngle = dot(norm, lightDir);
    float diff = max(diffAngle, 0.0);
    
    // Apply a smoothstep to soften harsh transitions in diffuse lighting
    diff = smoothstep(0.0, 1.0, diff);
    
    // Ensure minimum diffuse light to reduce dark shadows at edges
    diff = max(diff, 0.2);
    
    vec3 diffuse = diffuseStrength * diff * lightColor;
    
    // Enhanced specular highlight using Blinn-Phong with smoother falloff
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    
    // Apply smoothing to specular highlight to prevent flickering
    spec = smoothstep(0.05, 1.0, spec);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Apply a brighter metallic tint
    vec3 metalColor = baseColor * vec3(1.05, 1.05, 1.05);
    
    // Combined lighting with reduced edge sensitivity
    vec3 result = (ambient + diffuse) * metalColor + specular;
    
    // Apply a post-process brightness boost to ensure model isn't too dark
    result = pow(result, vec3(0.85)); // Gamma adjustment to brighten
    
    // Final color
    FragColor = vec4(result, 1.0);
} 