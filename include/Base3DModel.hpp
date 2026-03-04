#ifndef BASE_3D_MODEL_HPP
#define BASE_3D_MODEL_HPP

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "imgui.h"
#include "Shader.hpp"
#include "meshing/Mesh.hpp"

/**
 * Base class for all 3D shape models providing standardized mesh handling
 * This class encapsulates common OpenGL buffer management, transformation matrices,
 * material properties, and mouse interaction patterns that all complex shapes should follow.
 */
class Base3DModel {
public:
    Base3DModel();
    virtual ~Base3DModel();
    
    // Core interface - must be implemented by derived classes
    virtual void generateMesh() = 0;
    
    // Common functionality provided by base class
    void render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);
    
    // Shader and material management
    void setShader(Shader* shader);
    void setMaterial(const glm::vec3& color, float ambient, float diffuse, float specular, float shininess);
    void setLight(const glm::vec3& position, const glm::vec3& color);
    void setRenderMode(int mode);
    
    // Mouse interaction (standardized across all shapes)
    void processMouseMovement(float xpos, float ypos);
    void processMousePress(bool pressed, float xpos, float ypos);
    void processMouseScroll(float yoffset);
    
    // View management (standardized camera positions)
    void resetView();
    void setFrontView();
    void setSideView();
    void setTopView();
    void setIsometricView();
    
    // Cross-section functionality (standardized across all shapes)
    void enableCrossSection(bool enable);
    void setCrossSectionAxis(int axis);
    void setCrossSectionPosition(float position);
    
    // FEM mesh generation and rendering
    virtual bool generateFEMMesh(float elementSize);
    void renderFEMMeshWireframe(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);
    void clearFEMMesh();
    bool hasFEMMesh() const { return !femMesh.isEmpty(); }
    void setShowFEMMesh(bool show) { showFEMMesh = show; }
    bool getShowFEMMesh() const { return showFEMMesh; }
    float getFEMElementSize() const { return femElementSize; }
    void setFEMElementSize(float size) { femElementSize = size; }
    const Core::Meshing::Mesh& getFEMMesh() const { return femMesh; }

    // Status queries
    bool isMouseDragging() const { return mouseDragging; }
    bool hasMesh() const { return !indices.empty(); }
    
protected:
    // OpenGL buffer management
    void setupMesh();
    void updateModelMatrix();
    
    // Mesh generation helpers (common patterns)
    void revolveProfile(const std::vector<glm::dvec2>& profile, int segments, float yOffset = 0.0f, float yScale = 1.0f);
    void revolveProfileAroundX(const std::vector<glm::dvec2>& profile, int segments, float xOffset = 0.0f, float xScale = 1.0f);
    void generateNormals();
    void clearMesh();
    void setupMeshWireframeBuffers();
    
    // OpenGL objects
    unsigned int VAO, VBO, EBO;
    
    // Mesh data
    std::vector<float> vertices;      // Format: [x, y, z, nx, ny, nz] per vertex
    std::vector<unsigned int> indices;
    
    // Shader reference
    Shader* shader;
    
    // Transformation matrix and parameters
    glm::mat4 model;
    float modelScale;
    float modelRotationX;
    float modelRotationY;
    
    // Material properties (standardized lighting model)
    glm::vec3 objectColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float shininess;
    
    // Light properties
    glm::vec3 lightPos;
    glm::vec3 lightColor;
    
    // Render settings
    int renderMode;
    
    // Mouse interaction state
    bool mouseDragging;
    float lastX, lastY;
    
    // Cross-section settings
    bool showCrossSection;
    int crossSectionAxis;
    float crossSectionPos;
    
    // FEM mesh data
    Core::Meshing::Mesh femMesh;
    bool showFEMMesh;
    float femElementSize;
    unsigned int meshWireVAO, meshWireVBO;
    int meshWireVertexCount;
    
private:
    // Internal helper methods
    void applyCrossSectionClipping();
    void setCommonShaderUniforms(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);
};

#endif
