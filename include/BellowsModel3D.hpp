#ifndef BELLOWS_MODEL_3D_HPP
#define BELLOWS_MODEL_3D_HPP

#include "Base3DModel.hpp"
#include "shapes/ComplexShapes.hpp"
#include "fem/BellowsFEMAnalysis.hpp"
#include "fem/Material.hpp"

/**
 * BellowsModel3D - Standardized 3D model for bellows shapes
 * Inherits from Base3DModel to follow unified architecture pattern
 */
class BellowsModel3D : public Base3DModel {
public:
    BellowsModel3D();
    ~BellowsModel3D() = default;
    
    // Base3DModel interface implementation
    void generateMesh() override;
    bool generateFEMMesh(float elementSize) override;
    
    // Bellows-specific mesh generation
    void generateMesh(const Drawing::Bellows* bellows);

    // FEM analysis
    bool runFEMAnalysis(const Core::FEM::AnalysisConfig& config);
    bool hasFEMResult() const { return femResult.isValid; }
    const Core::FEM::FEMResult& getFEMResult() const { return femResult; }
    void clearFEMResult();

    // Stress visualization
    void setShowStressContours(bool show) { showStressContours = show; }
    bool getShowStressContours() const { return showStressContours; }
    void renderStressContours(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);

    // Deformed shape overlay
    void setShowDeformed(bool show) { showDeformed = show; }
    bool getShowDeformed() const { return showDeformed; }
    
    enum class MeshType { TRIANGULAR = 0, QUADRILATERAL = 1 };
    void setMeshType(MeshType type) { meshType = type; generateFEMMesh(femElementSize); }
    MeshType getMeshType() const { return meshType; }
    void setDeformScale(float s) { deformScale = s; setupDeformedBuffers(); }
    float getDeformScale() const { return deformScale; }
    void renderDeformed(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);

    // Modal analysis
    bool runModalAnalysis(int numModes);
    bool hasModalResult() const { return modalResult.isValid; }
    const Core::FEM::ModalResult& getModalResult() const { return modalResult; }
    void clearModalResult() { modalResult = Core::FEM::ModalResult(); activeMode = -1; cleanupModalBuffers(); }
    void setActiveMode(int mode);
    int getActiveMode() const { return activeMode; }
    void renderModeShape(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);

    // Mesh convergence study
    bool runConvergenceStudy(const Core::FEM::AnalysisConfig& config, int numPoints = 5);
    const std::vector<Core::FEM::ConvergencePoint>& getConvergenceData() const { return convergenceData; }
    bool hasConvergenceData() const { return !convergenceData.empty(); }
    void clearConvergenceData() { convergenceData.clear(); }

    // Material for FEM
    void setFEMMaterial(const Core::FEM::Material& mat) { femMaterial = mat; }
    const Core::FEM::Material& getFEMMaterial() const { return femMaterial; }

    // Access current bellows
    const Drawing::Bellows* getCurrentBellows() const { return currentBellows; }

private:
    const Drawing::Bellows* currentBellows;
    
    void generateBellowsGeometry(const Drawing::Bellows* bellows);

    // FEM data
    Core::FEM::FEMResult femResult;
    Core::FEM::Material femMaterial;
    MeshType meshType = MeshType::TRIANGULAR;
    bool showStressContours = false;

    // OpenGL stress visualization buffers
    unsigned int stressVAO = 0, stressVBO = 0;
    int stressVertexCount = 0;
    void setupStressBuffers();
    void cleanupStressBuffers();
    glm::vec3 stressColor(double value, double minVal, double maxVal) const;

    // Deformed shape overlay data
    bool showDeformed = false;
    float deformScale = 100.0f;
    unsigned int deformedVAO = 0, deformedVBO = 0;
    int deformedVertexCount = 0;
    void setupDeformedBuffers();
    void cleanupDeformedBuffers();

    // Modal analysis data
    Core::FEM::ModalResult modalResult;
    int activeMode = -1;
    unsigned int modalVAO = 0, modalVBO = 0;
    int modalVertexCount = 0;
    void setupModalBuffers();
    void cleanupModalBuffers();

    // Convergence study data
    std::vector<Core::FEM::ConvergencePoint> convergenceData;
};

#endif 