#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

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

// Render mode
uniform int renderMode; // 0=solid, 1=wireframe, 2=textured

void main() {
    // Ambient lighting
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;
    
    // Specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combined lighting effect
    vec3 result = (ambient + diffuse + specular) * objectColor;
    
    // Apply render mode effects
    if (renderMode == 1) { // Wireframe mode
        // Create a grid effect based on fragment position
        float gridSize = 0.05;
        vec3 gridColor = vec3(0.8, 0.8, 0.8);
        
        // Calculate grid lines
        float gridX = abs(fract(FragPos.x / gridSize) - 0.5);
        float gridY = abs(fract(FragPos.y / gridSize) - 0.5);
        float gridZ = abs(fract(FragPos.z / gridSize) - 0.5);
        
        float gridLine = max(max(gridX, gridY), gridZ);
        float gridFactor = smoothstep(0.45, 0.5, gridLine);
        
        // Mix the phong shading with the grid
        result = mix(gridColor, result, gridFactor);
    }
    
    FragColor = vec4(result, 1.0);
} 