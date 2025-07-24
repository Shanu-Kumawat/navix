#include "BallBearingModel3D.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

BallBearingModel3D::BallBearingModel3D() 
    : VAO(0), VBO(0), EBO(0), 
      shader(nullptr), 
      modelScale(1.0f), 
      modelRotationX(0.0f), 
      modelRotationY(0.0f),
      objectColor(0.9f, 0.9f, 0.95f), // High-quality metallic appearance
      ambientStrength(0.2f),           // Reduced ambient for better contrast
      diffuseStrength(0.8f),           // Increased diffuse for sharper features
      specularStrength(0.9f),          // High specularity for metallic look
      shininess(128.0f),               // Higher shininess for sharp reflections
      lightPos(1.0f, 1.0f, 2.0f),
      lightColor(1.0f, 1.0f, 1.0f),
      renderMode(0),
      mouseDragging(false),
      lastX(0.0f), 
      lastY(0.0f),
      showCrossSection(false),
      crossSectionAxis(0),
      crossSectionPos(0.0f) {
    
    model = glm::mat4(1.0f); // Initialize model matrix like bellows
}

BallBearingModel3D::~BallBearingModel3D() {
    // Clean up buffers
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void BallBearingModel3D::generateMesh(const Drawing::BallBearing* ballBearing) {
    if (!ballBearing) return;
    
    // Clear any existing mesh data
    vertices.clear();
    indices.clear();
    
    // Get parameters and normalize to reasonable scale
    float outerRadius = ballBearing->outerDiameter / 2.0f;
    float innerRadius = ballBearing->innerDiameter / 2.0f;
    float width = ballBearing->width;
    
    // Normalize to [-1, 1] range like bellows for consistent OpenGL rendering
    float maxDimension = std::max(outerRadius, width);
    float scale = 1.0f / maxDimension;
    
    // Generate race geometry using profile revolution like bellows (higher quality)
    generateRaceProfile(innerRadius * scale, outerRadius * scale, width * scale, 72); // Increased from 36 to 72 segments
    
    // Generate ball geometry if enabled
    if (ballBearing->showBalls) {
        std::vector<ImVec2> ballPositions = ballBearing->generateBallPositions();
        float ballRadius = ballBearing->ballDiameter / 2.0f * scale * 1.5f; // Make balls 50% larger for visibility
        
        generateBallGeometry(ballPositions, ballRadius, width * scale, scale);
    }
    
    // Calculate normals like bellows (proper face normal calculation)
    generateNormals();
    
    // Setup OpenGL buffers
    setupMesh();
    
    // Reset view
    resetView();
}

void BallBearingModel3D::generateRaceProfile(float innerRadius, float outerRadius, float width, int segments) {
    // Use the new separate races approach to avoid filled space between races
    generateSeparateRaces(innerRadius, outerRadius, width, segments);
}

void BallBearingModel3D::generateSeparateRaces(float innerRadius, float outerRadius, float width, int segments) {
    float halfWidth = width / 2.0f;
    
    // Calculate race dimensions
    float raceWidth = (outerRadius - innerRadius) * 0.18f;  // 18% of gap for race thickness
    float innerRaceOuter = innerRadius + raceWidth;
    float outerRaceInner = outerRadius - raceWidth;
    
    // Race height and groove details
    float raceHeight = halfWidth * 0.7f;  // Race height is 70% of total width
    float grooveDepth = raceWidth * 0.15f; // Groove depth for ball contact
    
    // Generate INNER RACE ONLY (hollow design)
    std::vector<ImVec2> innerRaceProfile;
    
    // Inner race with ball groove - create a proper ring shape (clockwise from bottom)
    innerRaceProfile.push_back(ImVec2(innerRadius, -raceHeight));      // Start: inner bottom
    innerRaceProfile.push_back(ImVec2(innerRadius, raceHeight));       // Inner top
    innerRaceProfile.push_back(ImVec2(innerRaceOuter - grooveDepth, raceHeight)); // Race top start
    
    // Inner race ball groove (curved top to bottom)
    float innerGrooveMidRadius = (innerRadius + innerRaceOuter) / 2.0f;
    innerRaceProfile.push_back(ImVec2(innerGrooveMidRadius, raceHeight * 0.8f));  // Groove top
    innerRaceProfile.push_back(ImVec2(innerGrooveMidRadius, 0.0f));               // Groove center
    innerRaceProfile.push_back(ImVec2(innerGrooveMidRadius, -raceHeight * 0.8f)); // Groove bottom
    
    innerRaceProfile.push_back(ImVec2(innerRaceOuter - grooveDepth, -raceHeight)); // Race bottom start
    
    // Close the loop back to start
    innerRaceProfile.push_back(ImVec2(innerRadius, -raceHeight));      // Back to start
    
    // Generate inner race geometry
    revolveProfile(innerRaceProfile, segments);
    
    // Generate OUTER RACE ONLY (separate from inner race - hollow space between)
    std::vector<ImVec2> outerRaceProfile;
    
    // Outer race with ball groove - create a proper ring shape (clockwise from bottom)
    outerRaceProfile.push_back(ImVec2(outerRaceInner + grooveDepth, -raceHeight)); // Start: inner edge bottom
    
    // Outer race ball groove (curved bottom to top)
    float outerGrooveMidRadius = (outerRaceInner + outerRadius) / 2.0f;
    outerRaceProfile.push_back(ImVec2(outerGrooveMidRadius, -raceHeight * 0.8f)); // Groove bottom
    outerRaceProfile.push_back(ImVec2(outerGrooveMidRadius, 0.0f));               // Groove center
    outerRaceProfile.push_back(ImVec2(outerGrooveMidRadius, raceHeight * 0.8f));  // Groove top
    
    outerRaceProfile.push_back(ImVec2(outerRaceInner + grooveDepth, raceHeight)); // Inner edge top
    outerRaceProfile.push_back(ImVec2(outerRadius, raceHeight));       // Outer top
    outerRaceProfile.push_back(ImVec2(outerRadius, -raceHeight));      // Outer bottom
    
    // Close the loop back to start
    outerRaceProfile.push_back(ImVec2(outerRaceInner + grooveDepth, -raceHeight)); // Back to start
    
    // Generate outer race geometry  
    revolveProfile(outerRaceProfile, segments);
}

void BallBearingModel3D::revolveProfile(const std::vector<ImVec2>& profile, int segments) {
    if (profile.empty()) return;
    
    const float PI = 3.14159265359f;
    const float angleStep = 2.0f * PI / segments;
    
    // Store the starting vertex index for this profile
    unsigned int baseVertexIndex = vertices.size() / 6; // 6 floats per vertex (pos + normal)
    
    // Generate vertices by revolving profile around Y-axis
    for (const auto& point : profile) {
        float radius = point.x;  // X becomes radius
        float height = point.y;  // Y becomes height
        
        // Generate vertices around the ring
        for (int i = 0; i < segments; ++i) {
            float angle = i * angleStep;
            
            // Position: revolve around Y-axis
            float vx = radius * cos(angle);
            float vy = height;
            float vz = radius * sin(angle);
            
            // Add vertex (position + placeholder for normal)
            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
            
            // Add placeholder for normal vector (calculated later)
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }
    
    // Generate indices for triangles (accounting for existing vertices)
    for (size_t i = 0; i < profile.size() - 1; ++i) {
        for (int j = 0; j < segments; ++j) {
            // Calculate vertex indices relative to this profile's base
            unsigned int current = baseVertexIndex + i * segments + j;
            unsigned int next = baseVertexIndex + i * segments + (j + 1) % segments;
            unsigned int currentNext = baseVertexIndex + (i + 1) * segments + j;
            unsigned int nextNext = baseVertexIndex + (i + 1) * segments + (j + 1) % segments;
            
            // First triangle (current, next, currentNext)
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(currentNext);
            
            // Second triangle (next, nextNext, currentNext)
            indices.push_back(next);
            indices.push_back(nextNext);
            indices.push_back(currentNext);
        }
    }
}

void BallBearingModel3D::generateBallGeometry(const std::vector<ImVec2>& ballPositions, float ballRadius, float width, float scale) {
    unsigned int baseIndex = vertices.size() / 6; // 6 floats per vertex (pos + normal)
    
    int ballSegments = 20; // Increase for much better sphere quality
    
    for (const auto& ballPos : ballPositions) {
        // Ball positions are already in world coordinates, so scale them to match the race scaling
        float ballX = ballPos.x * scale;
        float ballZ = ballPos.y * scale;
        
        // Generate a high-quality sphere for each ball
        for (int i = 0; i <= ballSegments; ++i) {
            float phi = (float)i / ballSegments * M_PI; // 0 to PI (latitude)
            
            for (int j = 0; j <= ballSegments; ++j) {
                float theta = (float)j / ballSegments * 2.0f * M_PI; // 0 to 2*PI (longitude)
                
                // Spherical coordinates to Cartesian with better positioning
                float x = ballX + ballRadius * sin(phi) * cos(theta);
                float y = ballRadius * cos(phi) * 0.9f; // Slightly compressed to fit in groove
                float z = ballZ + ballRadius * sin(phi) * sin(theta);
                
                // Add vertex (position + placeholder for normal)
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                
                // Add placeholder for normal vector (calculated later)
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
        }
        
        // Generate indices for this ball (sphere)
        for (int i = 0; i < ballSegments; ++i) {
            for (int j = 0; j < ballSegments; ++j) {
                unsigned int first = baseIndex + i * (ballSegments + 1) + j;
                unsigned int second = baseIndex + (i + 1) * (ballSegments + 1) + j;
                
                // Create two triangles for each quad
                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);
                
                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }
        
        baseIndex += (ballSegments + 1) * (ballSegments + 1);
    }
}

void BallBearingModel3D::generateNormals() {
    // Reset all normals to zero
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        vertices[i * 6 + 3] = 0.0f;
        vertices[i * 6 + 4] = 0.0f;
        vertices[i * 6 + 5] = 0.0f;
    }
    
    // Calculate normals for each face and accumulate
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        // Get positions of the three vertices
        glm::vec3 v0(vertices[i0 * 6], vertices[i0 * 6 + 1], vertices[i0 * 6 + 2]);
        glm::vec3 v1(vertices[i1 * 6], vertices[i1 * 6 + 1], vertices[i1 * 6 + 2]);
        glm::vec3 v2(vertices[i2 * 6], vertices[i2 * 6 + 1], vertices[i2 * 6 + 2]);
        
        // Calculate face normal using cross product
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        // Add to the normals of all three vertices
        vertices[i0 * 6 + 3] += normal.x;
        vertices[i0 * 6 + 4] += normal.y;
        vertices[i0 * 6 + 5] += normal.z;
        
        vertices[i1 * 6 + 3] += normal.x;
        vertices[i1 * 6 + 4] += normal.y;
        vertices[i1 * 6 + 5] += normal.z;
        
        vertices[i2 * 6 + 3] += normal.x;
        vertices[i2 * 6 + 4] += normal.y;
        vertices[i2 * 6 + 5] += normal.z;
    }
    
    // Normalize all vertex normals
    for (size_t i = 0; i < vertices.size() / 6; ++i) {
        glm::vec3 normal(vertices[i * 6 + 3], vertices[i * 6 + 4], vertices[i * 6 + 5]);
        if (glm::length(normal) > 0.0f) {
            normal = glm::normalize(normal);
            vertices[i * 6 + 3] = normal.x;
            vertices[i * 6 + 4] = normal.y;
            vertices[i * 6 + 5] = normal.z;
        }
    }
}

void BallBearingModel3D::setupMesh() {
    // Create buffers if they don't exist
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }
    
    // Bind buffers and upload data
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Set vertex attribute pointers
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Unbind VAO
    glBindVertexArray(0);
}

void BallBearingModel3D::render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos) {
    if (!shader || VAO == 0 || indices.empty()) return;
    
    // Use shader
    shader->use();
    
    // Set matrices
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    
    // Set material properties
    shader->setVec3("objectColor", objectColor);
    shader->setFloat("ambientStrength", ambientStrength);
    shader->setFloat("diffuseStrength", diffuseStrength);
    shader->setFloat("specularStrength", specularStrength);
    shader->setFloat("shininess", shininess);
    
    // Set light properties
    shader->setVec3("lightPos", lightPos);
    shader->setVec3("lightColor", lightColor);
    shader->setVec3("viewPos", cameraPos);
    
    // Set render mode
    shader->setInt("renderMode", renderMode);
    
    // Draw mesh
    glBindVertexArray(VAO);
    
    if (showCrossSection) {
        // Setup cross-section clipping plane
        glm::vec4 clipPlane;
        switch (crossSectionAxis) {
            case 0: // X-axis
                clipPlane = glm::vec4(1.0f, 0.0f, 0.0f, -crossSectionPos);
                break;
            case 1: // Y-axis
                clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, -crossSectionPos);
                break;
            case 2: // Z-axis
                clipPlane = glm::vec4(0.0f, 0.0f, 1.0f, -crossSectionPos);
                break;
            default:
                clipPlane = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                break;
        }
        
        glEnable(GL_CLIP_DISTANCE0);
        shader->setVec4("clipPlane", clipPlane);
    } else {
        glDisable(GL_CLIP_DISTANCE0);
    }
    
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void BallBearingModel3D::setShader(Shader* shader) {
    this->shader = shader;
}

void BallBearingModel3D::setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininess) {
    this->objectColor = color;
    this->ambientStrength = ambient;
    this->diffuseStrength = diffuse;
    this->specularStrength = specular;
    this->shininess = shininess;
}

void BallBearingModel3D::setLight(const glm::vec3& position, const glm::vec3& color) {
    this->lightPos = position;
    this->lightColor = color;
}

void BallBearingModel3D::setRenderMode(int mode) {
    this->renderMode = mode;
}

void BallBearingModel3D::processMouseMovement(float xpos, float ypos) {
    if (mouseDragging) {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        
        const float sensitivity = 0.5f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;
        
        modelRotationY += xoffset;
        modelRotationX += yoffset;
        
        // Constrain pitch
        if (modelRotationX > 89.0f) modelRotationX = 89.0f;
        if (modelRotationX < -89.0f) modelRotationX = -89.0f;
        
        updateModelMatrix();
    }
    
    lastX = xpos;
    lastY = ypos;
}

void BallBearingModel3D::processMousePress(bool pressed, float xpos, float ypos) {
    mouseDragging = pressed;
    lastX = xpos;
    lastY = ypos;
}

void BallBearingModel3D::processMouseScroll(float yoffset) {
    modelScale += yoffset * 0.1f;
    if (modelScale < 0.1f) modelScale = 0.1f;
    if (modelScale > 10.0f) modelScale = 10.0f;
    updateModelMatrix();
}

void BallBearingModel3D::resetView() {
    modelScale = 1.0f;
    modelRotationX = 0.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}

void BallBearingModel3D::setFrontView() {
    modelRotationX = 0.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}

void BallBearingModel3D::setSideView() {
    modelRotationX = 0.0f;
    modelRotationY = 90.0f;
    updateModelMatrix();
}

void BallBearingModel3D::setTopView() {
    modelRotationX = -90.0f;
    modelRotationY = 0.0f;
    updateModelMatrix();
}

void BallBearingModel3D::setIsometricView() {
    modelRotationX = -30.0f;
    modelRotationY = -45.0f;
    updateModelMatrix();
}

void BallBearingModel3D::updateModelMatrix() {
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(modelScale));
    model = glm::rotate(model, glm::radians(modelRotationX), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
}

void BallBearingModel3D::enableCrossSection(bool enable) {
    showCrossSection = enable;
}

void BallBearingModel3D::setCrossSectionAxis(int axis) {
    crossSectionAxis = axis;
}

void BallBearingModel3D::setCrossSectionPosition(float position) {
    crossSectionPos = position;
}
