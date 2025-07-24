#ifndef BALL_BEARING_MODEL_3D_HPP
#define BALL_BEARING_MODEL_3D_HPP

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "imgui.h"
#include "Shader.hpp"
#include "shapes/ComplexShapes.hpp"

class BallBearingModel3D {
public:
    // Constructor
    BallBearingModel3D();
    
    // Destructor
    ~BallBearingModel3D();
    
    // Generate 3D mesh from ball bearing
    void generateMesh(const Drawing::BallBearing* ballBearing);
    
    // Render the 3D model
    void render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);
    
    // Set shader and material properties
    void setShader(Shader* shader);
    void setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininess);
    void setLight(const glm::vec3& position, const glm::vec3& color);
    void setRenderMode(int mode);
    
    // Mouse interaction handlers
    void processMouseMovement(float xpos, float ypos);
    void processMousePress(bool pressed, float xpos, float ypos);
    void processMouseScroll(float yoffset);
    
    // View management
    void resetView();
    void setFrontView();
    void setSideView();
    void setTopView();
    void setIsometricView();
    
    // Cross-section view
    void enableCrossSection(bool enable);
    void setCrossSectionAxis(int axis);
    void setCrossSectionPosition(float position);
    
    // Get mouse interaction status
    bool isMouseDragging() const { return mouseDragging; }
    
    // Update model transformation
    void updateModelMatrix();
    
private:
    // OpenGL objects
    unsigned int VAO, VBO, EBO;
    
    // Model data
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Shader
    Shader* shader;
    
    // Model transform
    glm::mat4 model;
    float modelScale;
    float modelRotationX;
    float modelRotationY;
    
    // Material properties
    glm::vec3 objectColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float shininess;
    
    // Light properties
    glm::vec3 lightPos;
    glm::vec3 lightColor;
    
    // Render mode
    int renderMode;
    
    // Mouse interaction
    bool mouseDragging;
    float lastX, lastY;
    
    // Cross-section settings
    bool showCrossSection;
    int crossSectionAxis;
    float crossSectionPos;
    
    // Helper methods
    void generateRaceProfile(float innerRadius, float outerRadius, float width, int segments);
    void revolveProfile(const std::vector<ImVec2>& profile, int segments);
    void generateBallGeometry(const std::vector<ImVec2>& ballPositions, float ballRadius, float width, float scale);
    void generateNormals();
    void setupMesh();
};

#endif
