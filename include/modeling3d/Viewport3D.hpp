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
    Boolean,        // Boolean operations on solid bodies
    MoveBody,       // Position a body: translate / coincident face mate / rotate
    Combine,        // SolidWorks Combine: Add / Subtract / Common (N bodies)
    CutExtrude,     // Cut-Extrude / Pocket (sketch profile cut into a body)
    Hole,           // Hole wizard lite (click a face, parametric hole)
    Pattern,        // Linear / circular pattern of the selected body
    Mirror,         // Mirror the selected body about a plane
    Move,
    Rotate,
    Scale,
    PlacePrimitive  // NEW: waiting for user to click canvas placement point
};
// CutDirMode / HoleType / PatternKind enums live in Body3D.hpp (shared with
// Modeling3DScene, which cannot include Viewport3D.hpp)

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
    void setActiveTool(Tool3DType tool);
    SketchDrawTool getSketchDrawTool() const { return sketchDrawTool; }
    void setSketchDrawTool(SketchDrawTool t) { sketchDrawTool = t; }

    // Render mode
    RenderMode3D getRenderMode() const { return renderMode; }
    void setRenderMode(RenderMode3D mode) { renderMode = mode; }

    // Work plane
    void setWorkPlane(const WorkPlane3D& wp);

    // Sketch plane selection (SolidWorks-style: choose the plane BEFORE
    // drawing). 0 = XZ ground (default), 1 = XY front, 2 = YZ side.
    int getSketchPlaneIndex() const { return sketchPlaneIndex; }
    void setSketchPlaneIndex(int idx);
    WorkPlane3D sketchPlaneForIndex(int idx);
    void beginSketchOnPendingPlane();   // starts a sketch on the chosen plane
    // Cancel an in-progress face sketch owned by Cut-Extrude

    // Extrude/Revolve
    float getExtrudeDistance() const { return extrudeDistance; }
    void setExtrudeDistance(float d);
    float getRevolveAngle() const { return revolveAngle; }
    void setRevolveAngle(float a);
    RevolveAxis getRevolveAxisMode() const { return revolveAxisMode; }
    void setRevolveAxisMode(RevolveAxis m);

    bool executeExtrude();
    bool executeRevolve();

    // Fillet / Chamfer
    float getFilletRadius() const { return filletRadius; }
    void setFilletRadius(float r);
    float getChamferDistance() const { return chamferDistance; }
    void setChamferDistance(float d);

    // Boolean operations (base body is modified, tool body is consumed)
    BoolOp getBoolOpMode() const { return boolOpMode; }
    void setBoolOpMode(BoolOp op);
    void beginBoolean(BoolOp op);        // activate tool; adopts selected body as base
    Body3D* getBoolBaseBody() const { return boolBaseBody; }
    Body3D* getBoolToolBody() const { return boolToolBody; }
    void clearBooleanSelection();
    bool executeBoolean();

    // Merge (fuse N bodies into the first-picked one)
    void beginCombine();
    Body3D* getMergeBase() const { return mergeBase; }
    const std::vector<Body3D*>& getMergeTools() const { return mergeTools; }
    void clearMergeSelection();   // also clears combineFaceHL
    void mergeSelectAll();   // base = first visible body, tools = the rest
    bool executeCombine(BoolOp op);
    enum class MoveMode { Translate, CoincidentFace, Rotate };
    void beginMoveBody();
    void startMoveBodyPick() { movePickingBody = true; if (activeTool != Tool3DType::MoveBody) activeTool = Tool3DType::MoveBody; }
    bool isMovePickingBody() const { return movePickingBody; }
    Body3D* getMoveBody() const { return moveBody; }
    MoveMode getMoveMode() const { return moveMode; }
    void setMoveMode(MoveMode m);
    glm::dvec3 getMoveDelta() const { return moveDelta; }
    void setMoveDelta(const glm::dvec3& d) { moveDelta = d; }
    bool isMoveFacePicked() const { return moveFacePicked; }
    // Coplanar-patch face highlight (world-space, lifted triangles)
    std::vector<float> moveFaceHL, moveTgtHL, combineFaceHL;
    bool isMoveTargetPicked() const { return moveTargetPicked; }
    Body3D* getMoveTargetBody() const { return moveTargetBody; }
    void clearMoveFaces() { moveFacePicked = false; moveTargetPicked = false; moveTargetBody = nullptr; moveFaceHL.clear(); moveTgtHL.clear(); }
    int getRotateAxis() const { return rotateAxis; }
    void setRotateAxis(int a) { rotateAxis = a; }
    float getRotateAngle() const { return rotateAngle; }
    void setRotateAngle(float a) { rotateAngle = a; }
    bool executeMoveBody();
    glm::dmat4 moveCoincidentTransform() const;
    glm::mat4 moveDestinationTransform() const;
    void renderFaceHighlights();
    // Build a highlight mesh for the coplanar face patch containing hitPt;
    // also returns the patch centroid (used for face-center alignment)
    void buildFacePatch(const Body3D* body, const glm::dvec3& hitPt,
                        const glm::dvec3& nrm, std::vector<float>& outVerts,
                        glm::dvec3& outCentroid);

    glm::mat4 mergeToolPlacement(Body3D* tool) const;

    // Face-targeted merge: pick a face of the base, tools are MOVED onto that
    // face (SolidWorks Move/Copy Body + Combine, condensed into one flow)
    bool isMergeFacePicked() const { return mergeHasFace; }
    int getMergeAlign() const { return mergeAlign; }   // 0=Flat on face, 1=Centered on point
    void setMergeAlign(int a) { if (mergeAlign != a) { mergeAlign = a; rebuildToolPreview(); } }
    float getMergeOffset() const { return mergeOffset; }
    void setMergeOffset(float o) { if (std::abs(mergeOffset - o) > 1e-4) { mergeOffset = o; rebuildToolPreview(); } }
    void clearMergeFace() { mergeHasFace = false; rebuildToolPreview(); }

    // Cut-Extrude / Pocket (pick body → pick its face → draw on the face grid)
    int getCutStep() const { return cutStep; }
    Body3D* getCutTarget() const { return cutTarget; }
    CutDirMode getCutDirMode() const { return cutDirMode; }
    void setCutDirMode(CutDirMode m) { if (cutDirMode != m) { cutDirMode = m; rebuildToolPreview(); } }
    float getCutDepth() const { return cutDepth; }
    void setCutDepth(float d) { if (std::abs(cutDepth - d) > 1e-4) { cutDepth = d; rebuildToolPreview(); } }
    bool executeCutExtrude();

    // Hole wizard lite (target body must be selected, then click its face)
    void beginHole();
    bool isHolePicked() const { return holePicked; }
    glm::dvec3 getHoleCenter() const { return holeCenter; }
    HoleType getHoleType() const { return holeType; }
    void setHoleType(HoleType t) { if (holeType != t) { holeType = t; rebuildToolPreview(); } }
    float getHoleDiameter() const { return holeDia; }
    void setHoleDiameter(float d) { if (std::abs(holeDia - d) > 1e-4) { holeDia = d; rebuildToolPreview(); } }
    float getHoleDepth() const { return holeDepth; }
    void setHoleDepth(float d) { if (std::abs(holeDepth - d) > 1e-4) { holeDepth = d; rebuildToolPreview(); } }
    float getCBoreDiameter() const { return cboreDia; }
    void setCBoreDiameter(float d) { if (std::abs(cboreDia - d) > 1e-4) { cboreDia = d; rebuildToolPreview(); } }
    float getCBoreDepthValue() const { return cboreDepth; }
    void setCBoreDepth(float d) { if (std::abs(cboreDepth - d) > 1e-4) { cboreDepth = d; rebuildToolPreview(); } }
    void clearHolePick() { holePicked = false; rebuildToolPreview(); }
    bool executeHole();

    // Pattern (linear / circular copies of the selected body)
    PatternKind getPatternKind() const { return patternKind; }
    void setPatternKind(PatternKind k) { if (patternKind != k) { patternKind = k; rebuildToolPreview(); } }
    int getLinearAxis() const { return linAxis; }
    void setLinearAxis(int a) { if (linAxis != a) { linAxis = a; rebuildToolPreview(); } }
    int getLinearCount() const { return linCount; }
    void setLinearCount(int n) { if (linCount != n && n >= 2) { linCount = n; rebuildToolPreview(); } }
    float getLinearSpacing() const { return linSpacing; }
    void setLinearSpacing(float s) { if (std::abs(linSpacing - s) > 1e-4) { linSpacing = s; rebuildToolPreview(); } }
    int getCircAxis() const { return circAxis; }
    void setCircAxis(int a) { if (circAxis != a) { circAxis = a; rebuildToolPreview(); } }
    int getCircCenter() const { return circCenter; }   // 0=world origin, 1=source center, 2=picked body center
    void setCircCenter(int c) { if (circCenter != c) { circCenter = c; rebuildToolPreview(); } }
    Body3D* getCircCenterBody() const { return circCenterBody; }
    void clearCircCenterBody() { circCenterBody = nullptr; rebuildToolPreview(); }
    int getCircCount() const { return circCount; }
    void setCircCount(int n) { if (circCount != n && n >= 2) { circCount = n; circCount = std::min(circCount, 64); rebuildToolPreview(); } }
    bool executePattern();

    // Mirror (selected body about a global plane)
    int getMirrorPlane() const { return mirrorPlane; }
    void setMirrorPlane(int p) { if (mirrorPlane != p) { mirrorPlane = p; rebuildToolPreview(); } }
    float getMirrorOffset() const { return mirrorOffset; }
    void setMirrorOffset(float o) { if (std::abs(mirrorOffset - o) > 1e-4) { mirrorOffset = o; rebuildToolPreview(); } }
    bool getMirrorFuse() const { return mirrorFuse; }
    void setMirrorFuse(bool f) { if (mirrorFuse != f) { mirrorFuse = f; rebuildToolPreview(); } }
    bool executeMirror();

    // Section view (clip bodies along an axis-aligned world plane)
    bool isSectionEnabled() const { return sectionEnabled; }
    void setSectionEnabled(bool v) { sectionEnabled = v; }
    int getSectionAxis() const { return sectionAxis; }   // 0=X 1=Y 2=Z
    void setSectionAxis(int a) { sectionAxis = a; }
    float getSectionPos() const { return sectionPos; }
    void setSectionPos(float p) { sectionPos = p; }
    bool isSectionFlipped() const { return sectionFlip; }
    void setSectionFlipped(bool f) { sectionFlip = f; }
    bool isCapDebugNoStencil() const { return capDebugNoStencil; }
    void setCapDebugNoStencil(bool v) { capDebugNoStencil = v; }

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

    // Primitive placement preview mode
    // Call these instead of createXxx directly: they enter PlacePrimitive mode.
    // On canvas click, the body is placed at the clicked world position.
    void startPlaceBox(double dx, double dy, double dz);
    void startPlaceCylinder(double radius, double height);
    int getCylinderAxis() const { return cylAxis; }   // 0=X 1=Y 2=Z
    void setCylinderAxis(int a) { cylAxis = a; if (isInPlacementMode()) rebuildToolPreview(); }
    void startPlaceSphere(double radius);
    void startPlaceCone(double r1, double r2, double height);
    void startPlaceTorus(double majorR, double minorR);
    void cancelPlacement();
    bool isInPlacementMode() const { return activeTool == Tool3DType::PlacePrimitive; }
    glm::dvec3 getPlacementCursorWorld() const { return placementCursorWorld; }

    // Fillet edge selection (public so PropertyPanel3D can display/clear)
    std::vector<int>  filletSelectedEdges;
    int               filletHoveredEdge{-1};

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

    // Sketch selection (for sketch-based extrude/revolve without needing active sketch)
    Sketch3D* getSelectedSketch() const { return selectedSketch; }
    void setSelectedSketch(Sketch3D* sk);
    Sketch3D* pickSketch(float screenX, float screenY) const; // returns closest sketch or nullptr

    // Preview generation (public so PropertyPanel3D can trigger it if needed)
    void rebuildToolPreview();
    std::unique_ptr<Body3D> toolPreviewBody;
    std::vector<std::unique_ptr<Body3D>> toolPreviewBodies;  // multi-ghost (pattern/hole)

    // State integrity (call sites: render loop, undo/redo, history changes)
    void validateToolReferences();   // drop dangling body/sketch pointers
    void onHistoryChanged();         // call after undo/redo: validate + rebuild previews


    // Spline: finalize accumulated control points → Catmull-Rom line segments
    void commitSpline();

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
    void renderToolPreview();
    void renderPlacementPreview();
    void renderFilletEdgeHighlights();  // GL overlay for hover/selected edges
    void renderWorkPlaneHighlight();
    void renderCutPlaneGrid();          // face-plane grid during Cut-Extrude drawing
    void rebuildCutGrid();
    void renderSectionPlane();          // translucent indicator for Section View
    glm::vec4 computeSectionPlane() const;  // camera-facing clip plane
    void renderOriginAxes();

    // Ray casting + projection
    glm::dvec3 screenToRay(float screenX, float screenY) const;
    Body3D* pickBody(float screenX, float screenY) const;
    glm::dvec2 screenToWorkPlane(float screenX, float screenY) const;
    glm::dvec2 worldToScreen(const glm::dvec3& worldPt) const; // 3D world -> canvas px

    // Per-tool cleanup when leaving a tool (single source of truth for
    // tool-switch state resets; called from setActiveTool)
    void deactivateToolState(Tool3DType previousTool);

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
    Tool3DType    activeTool{Tool3DType::Select};
    SketchDrawTool sketchDrawTool{SketchDrawTool::None};
    RenderMode3D  renderMode{RenderMode3D::SolidWithEdges};

    // Selected sketch (for sketch-based ops independent of active sketch)
    Sketch3D* selectedSketch{nullptr};

    // Pending sketch plane (chosen before drawing starts)
    int sketchPlaneIndex{0};   // 0=XZ ground, 1=XY front, 2=YZ side

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

    // Spline accumulation (Catmull-Rom multi-click)
    std::vector<glm::dvec2> splineControlPoints;  // accumulated clicks
    bool splineActive{false};                      // in multi-click spline mode

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

    // ── Primitive placement state ────────────────────────────────────────────
    enum class PrimType { None, Box, Cylinder, Sphere, Cone, Torus };
    PrimType  placePrimType{PrimType::None};
    double    placeP1{10}, placeP2{10}, placeP3{10};  // W/H/D or R/H etc.
    int       cylAxis{1};   // cylinder axis: 0=X 1=Y (up) 2=Z
    glm::dvec3 placementCursorWorld{0.0};  // world pos under mouse
    Body3D* commitPlacement(glm::dvec3 position); // create + place

    // ── Fillet edge selection (private helper methods) ────────────────────────
    void renderFilletHighlights();         // compat no-op
    int  pickEdge(float screenX, float screenY) const; // returns edge index or -1
    
    // Dynamic Tool Previews
    float filletRadius{1.0f};
    float chamferDistance{1.0f};

    // ── Boolean operation state ──────────────────────────────────────────────
    BoolOp  boolOpMode{BoolOp::Union};
    Body3D* boolBaseBody{nullptr};  // first picked — receives the result
    Body3D* boolToolBody{nullptr};  // second picked — consumed by the op

    // ── Merge state ──────────────────────────────────────────────────────────
    Body3D* mergeBase{nullptr};
    std::vector<Body3D*> mergeTools;

    // ── Move Body state ──────────────────────────────────────────────────────
    Body3D* moveBody{nullptr};
    bool movePickingBody{false};
    MoveMode moveMode{MoveMode::Translate};
    glm::dvec3 moveDelta{0.0};        // Translate mode (world XYZ)
    bool moveFacePicked{false};       // Coincident: mating face on moveBody
    glm::dvec3 moveFacePoint{0.0};
    glm::dvec3 moveFaceNormal{0.0, 1.0, 0.0};
    glm::dvec3 moveFaceCenter{0.0};   // coplanar patch centroid
    Body3D* moveTargetBody{nullptr};  // Coincident: the other body
    bool moveTargetPicked{false};
    glm::dvec3 moveTgtFacePoint{0.0};
    glm::dvec3 moveTgtFaceNormal{0.0, 1.0, 0.0};
    glm::dvec3 moveTgtCenter{0.0};    // coplanar patch centroid
    int rotateAxis{1};                // 0=X 1=Y 2=Z
    float rotateAngle{0.0f};          // degrees
    bool mergeHasFace{false};
    glm::dvec3 mergeFacePoint{0.0};
    glm::dvec3 mergeFaceNormal{0.0, 1.0, 0.0};
    int mergeAlign{0};          // 0 = flat on face, 1 = centered on point
    float mergeOffset{0.0f};    // distance above the face along its normal

    // ── Cut-Extrude / Hole / Pattern / Mirror state ──────────────────────────
    // Cut-Extrude flow: step 0 = pick target body, 1 = pick its face,
    // 2 = drawing the profile on the face plane grid
    int cutStep{0};
    Body3D* cutTarget{nullptr};
    WorkPlane3D cutPlane;              // plane of the picked face
    unsigned int cutGridVAO{0}, cutGridVBO{0};
    int cutGridLineCount{0};
    bool cutOwnsSketch{false};         // we began the face sketch ourselves

    CutDirMode cutDirMode{CutDirMode::Blind};
    float cutDepth{5.0f};

    bool holePicked{false};
    glm::dvec3 holeCenter{0.0};
    glm::dvec3 holeNormal{0.0, 1.0, 0.0};
    HoleType holeType{HoleType::Through};
    float holeDia{5.0f};
    float holeDepth{10.0f};
    float cboreDia{9.0f};
    float cboreDepth{3.0f};

    PatternKind patternKind{PatternKind::Linear};
    int linAxis{2};       // 0=X 1=Y 2=Z
    int linCount{3};
    float linSpacing{15.0f};
    int circAxis{2};      // 0=X 1=Y 2=Z
    int circCenter{0};    // 0=world origin, 1=source body center, 2=picked body center
    Body3D* circCenterBody{nullptr};  // used when circCenter == 2
    int circCount{6};

    int mirrorPlane{1};   // 0=YZ(x=0) 1=XZ(y=0) 2=XY(z=0)
    float mirrorOffset{0.0f};
    bool mirrorFuse{false};

    // Section view (axis-aligned world clip plane)
    bool sectionEnabled{false};
    int sectionAxis{1};   // 0=X 1=Y 2=Z
    float sectionPos{0.0f};
    bool sectionFlip{false};
    bool capDebugNoStencil{false};  // diagnostic: draw cap ignoring stencil

    // Ray-cast pick returning the hit point + face normal (Hole tool)
    Body3D* pickFace(float screenX, float screenY, glm::dvec3& outPoint,
                     glm::dvec3& outNormal) const;
};

} // namespace Modeling3D
