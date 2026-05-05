#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <functional>
#include "Camera.hpp"
#include "Shader.hpp"
#include "modeling3d/Modeling3DScene.hpp"
#include "modeling3d/WorkPlane3D.hpp"

namespace Modeling3D {

enum class RenderMode3D {
    Solid,
    SolidWithEdges,
    Wireframe
};

enum class Tool3DType {
    Select,
    Sketch,
    Extrude,
    Revolve,
    Fillet,
    Move,
    Rotate,
    Scale
};

enum class SketchDrawTool {
    None,
    Line,
    Rectangle,
    Circle,
    Spline
};

enum class StandardView {
    Front, Back, Left, Right, Top, Bottom, Isometric
};

// Geometric snap type — determines the visual indicator shown
enum class SnapType {
    None,
    Grid,      // Yellow dot
    Endpoint,  // Orange square
    Midpoint,  // Cyan triangle
    Center,    // Green circle
    Origin,    // White cross
};

// Revolve axis selection
enum class RevolveAxis {
    SketchX,    // X-direction of active work plane (horizontal)
    SketchY,    // Y-direction of active work plane (vertical)
    GlobalX,
    GlobalY,
    GlobalZ,
};

/**
 * @brief Full-screen 3D viewport with interactive CAD modeling.
 *
 * Professional-grade viewport matching SolidWorks/SpaceClaim aesthetics:
 * - Gradient background with subtle grid
 * - XYZ view triad (bottom-left) + clickable ViewCube (top-right)
 * - Interactive sketch drawing on work planes
 * - OCCT primitive creation
 * - ImGuizmo gizmo integration
 */
class Viewport3D {
public:
    Viewport3D();
    ~Viewport3D();

    void initialize();
    bool isInitialized() const { return initialized; }

    // Main render (to FBO)
    void render(int width, int height);

    // ImGui overlays (drawn ON TOP of the FBO image, from main.cpp)
    void renderViewTriad(float canvasX, float canvasY, float canvasW, float canvasH);
    void renderViewCube(float canvasX, float canvasY, float canvasW, float canvasH);
    void renderSnapOverlay(float canvasX, float canvasY);

    unsigned int getRenderedTexture() const { return colorTexture; }

    // Revolve axis
    RevolveAxis getRevolveAxisMode() const { return revolveAxisMode; }
    void setRevolveAxisMode(RevolveAxis m) { revolveAxisMode = m; }

    // Camera
    Camera* getCamera() { return camera.get(); }
    void resetCamera();
    void setStandardView(StandardView view);
    void zoomToFit();

    // Input
    void handleMouseButton(int button, bool pressed, float x, float y);
    void handleMouseMove(float x, float y);
    void handleMouseScroll(float yOffset);
    void handleKey(int key, bool pressed);

    // Scene
    Modeling3DScene* getScene() { return &scene; }
    const Modeling3DScene* getScene() const { return &scene; }

    // Tools
    Tool3DType getActiveTool() const { return activeTool; }
    void setActiveTool(Tool3DType tool) { activeTool = tool; }
    SketchDrawTool getSketchDrawTool() const { return sketchDrawTool; }
    void setSketchDrawTool(SketchDrawTool t) { sketchDrawTool = t; }

    // Render mode
    RenderMode3D getRenderMode() const { return renderMode; }
    void setRenderMode(RenderMode3D mode) { renderMode = mode; }

    // Work plane
    void setWorkPlane(const WorkPlane3D& wp);

    // Extrude/Revolve
    float getExtrudeDistance() const { return extrudeDistance; }
    void setExtrudeDistance(float d) { extrudeDistance = d; }
    float getRevolveAngle() const { return revolveAngle; }
    void setRevolveAngle(float a) { revolveAngle = a; }

    bool executeExtrude();
    bool executeRevolve();

    // Sketch chain mode
    bool isSketchChainMode() const { return sketchChainMode; }
    void setSketchChainMode(bool v) { sketchChainMode = v; }

    // Snap state
    SnapType getCurrentSnapType() const { return currentSnapType; }

    // OCCT Primitives
    Body3D* createBox(double dx, double dy, double dz);
    Body3D* createCylinder(double radius, double height);
    Body3D* createSphere(double radius);
    Body3D* createCone(double r1, double r2, double height);
    Body3D* createTorus(double majorR, double minorR);

    void markSketchDirty() { sketchDirty = true; }

    std::string getStatusText() const;

    // Snap controls
    bool  isSnapEnabled()   const { return snapEnabled; }
    void  setSnapEnabled(bool v)  { snapEnabled = v; }
    float getSnapGridSize() const { return snapGridSize; }
    void  setSnapGridSize(float s){ snapGridSize = s; }
    bool  isSnapActive()    const { return snapIsActive; }

    // Gizmo
    glm::mat4 getGizmoMatrix() const;
    void setGizmoMatrix(const glm::mat4& m);

    // Interactive sketch state
    bool isSketchDrawing() const { return sketchHasFirstPoint; }
    glm::dvec2 getSketchCursor() const { return sketchCursor2D; }

private:
    void setupFramebuffer(int width, int height);
    void resizeFramebuffer(int width, int height);

    // Render passes (inside FBO)
    void renderBackground();
    void renderGrid();
    void renderBodies();
    void renderSketch();
    void renderSketchPreview();
    void renderExtrudePreview();
    void renderWorkPlaneHighlight();
    void renderOriginAxes();

    // Ray casting + projection
    glm::dvec3 screenToRay(float screenX, float screenY) const;
    Body3D* pickBody(float screenX, float screenY) const;
    glm::dvec2 screenToWorkPlane(float screenX, float screenY) const;
    glm::dvec2 worldToScreen(const glm::dvec3& worldPt) const; // 3D world -> canvas px

    // Geometric snap
    glm::dvec2 findGeometricSnap(float mouseX, float mouseY,
                                 glm::dvec2 rawPt,
                                 SnapType& outType) const;

    // Sketch interaction
    void handleSketchClick(float x, float y);
    void handleSketchMove(float x, float y);
    void commitSketchShape();
    void rebuildSketchVAO();

    // State
    bool initialized{false};
    Modeling3DScene scene;
    std::unique_ptr<Camera> camera;

    // Shaders
    std::unique_ptr<Shader> modelShader;
    std::unique_ptr<Shader> gridShader;
    std::unique_ptr<Shader> bgShader;  // Gradient background

    // FBO
    unsigned int framebuffer{0};
    unsigned int colorTexture{0};
    unsigned int depthRenderbuffer{0};
    int fbWidth{0}, fbHeight{0};

    // VAOs
    unsigned int bgVAO{0}, bgVBO{0};     // Background quad
    unsigned int gridVAO{0}, gridVBO{0};
    unsigned int axisVAO{0}, axisVBO{0}; // World origin axes
    unsigned int sketchVAO{0}, sketchVBO{0};
    unsigned int previewVAO{0}, previewVBO{0};
    int sketchLineCount{0};
    bool sketchDirty{true};

    // Tools
    Tool3DType activeTool{Tool3DType::Select};
    SketchDrawTool sketchDrawTool{SketchDrawTool::None};
    RenderMode3D renderMode{RenderMode3D::SolidWithEdges};

    // Mouse state
    bool middleMouseDown{false};
    bool leftMouseDown{false};
    bool rightMouseDown{false};
    bool shiftDown{false};
    bool ctrlDown{false};
    float lastMouseX{0}, lastMouseY{0};

    // Sketch drawing state
    bool sketchHasFirstPoint{false};
    glm::dvec2 sketchFirstPoint{0.0};
    glm::dvec2 sketchCursor2D{0.0};

    // Operation params
    float extrudeDistance{10.0f};
    float revolveAngle{360.0f};

    // Snapping
    bool  snapEnabled{true};
    float snapGridSize{1.0f};
    bool  snapIsActive{false};
    SnapType  currentSnapType{SnapType::None};
    glm::dvec2 snapPoint2D{0.0};  // last snapped 2D position (for indicator)

    // Revolve
    RevolveAxis revolveAxisMode{RevolveAxis::SketchY};

    // Sketch chain: keep drawing after commit
    bool sketchChainMode{true};   // after placing 2nd point, restart from it

    // Lighting
    glm::vec3 lightPos{20.0f, 40.0f, 30.0f};
    glm::vec3 lightColor{1.0f, 0.98f, 0.95f};
};

} // namespace Modeling3D
