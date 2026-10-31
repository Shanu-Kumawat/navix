#pragma once

#include <vector>
#include <memory>
#include <string>
#include "modeling3d/Body3D.hpp"
#include "modeling3d/Sketch3D.hpp"
#include "modeling3d/WorkPlane3D.hpp"
#include "modeling3d/CommandManager3D.hpp"

namespace Modeling3D {

/**
 * @brief 3D scene graph managing bodies, sketches, and selection.
 *
 * This is the Model for the 3D modeling mode, analogous to Core::SceneModel for 2D.
 * Includes a CommandManager3D for undo/redo support.
 */
class Modeling3DScene {
public:
    Modeling3DScene();
    ~Modeling3DScene() = default;

    // Body management
    Body3D* addBody();
    void removeBody(Body3D* body);
    void clearAll();
    const std::vector<std::unique_ptr<Body3D>>& getBodies() const { return bodies; }
    Body3D* getBodyById(uint64_t id) const;

    // Body management for undo/redo (transfer ownership)
    void addExistingBody(std::unique_ptr<Body3D> body);
    std::unique_ptr<Body3D> detachBody(Body3D* body);

    // Selection
    Body3D* getSelectedBody() const { return selectedBody; }
    void selectBody(Body3D* body);
    void clearSelection();

    // Active sketch
    Sketch3D* getActiveSketch() const { return activeSketch; }
    void beginSketch(const WorkPlane3D& plane);
    void finishSketch();
    void cancelSketch();
    // Directly make an existing sketch the active one (for sketch-click → extrude workflow)
    void setActiveSketchDirect(Sketch3D* sk) {
        if (sk) { activeSketch = sk; activeWorkPlane = sk->getWorkPlane(); }
    }
    bool isSketchActive() const { return activeSketch != nullptr; }

    // Sketch storage
    const std::vector<std::unique_ptr<Sketch3D>>& getSketches() const { return sketches; }
    void removeSketch(Sketch3D* sk) {
        if (activeSketch == sk) activeSketch = nullptr;
        auto it = std::find_if(sketches.begin(), sketches.end(),
            [sk](const std::unique_ptr<Sketch3D>& s){ return s.get() == sk; });
        if (it != sketches.end()) sketches.erase(it);
    }

    // Work planes
    const WorkPlane3D& getActiveWorkPlane() const { return activeWorkPlane; }
    void setActiveWorkPlane(const WorkPlane3D& wp) { activeWorkPlane = wp; }

    // Operations on selected body + active sketch
    bool extrudeActiveSketch(const glm::dvec3& direction, double distance);
    bool revolveActiveSketch(const glm::dvec3& axisOrigin,
                             const glm::dvec3& axisDir,
                             double angleDegrees);

    // Boolean operation: base is modified in place, tool body is consumed.
    // Returns false if the op fails or the result would be empty.
    bool applyBoolean(Body3D* base, Body3D* tool, BoolOp op);

    // ── Tier-1 modeling features ─────────────────────────────────────────────
    // Cut a sketch profile into the target body (pocket / through-hole).
    // Cuts along `dir` for `dist`; `preShift` moves the profile start back
    // along -dir (used by Through-All to start just outside the face).
    // Returns false if nothing was removed.
    bool applyCutExtrude(Body3D* target, Sketch3D* sketch,
                         const glm::dvec3& dir, double dist, double preShift = 0.0);

    // Parametric hole at a picked face point; axis = -normal (into the body)
    bool applyHole(Body3D* target, const glm::dvec3& center, const glm::dvec3& normal,
                   HoleType type, double dia, double depth,
                   double cbDia, double cbDepth);

    // Linear / circular copies of the target (new bodies, undoable as one step)
    bool applyPattern(Body3D* target, PatternKind kind,
                      int linAxis, int linCount, double linSpacing,
                      int circAxis, int circCount,
                      const glm::dvec3& circCenterWorld = glm::dvec3(0));

    // Mirror the target about a global plane (0=YZ@x, 1=XZ@y, 2=XY@z);
    // optionally fuse the mirrored copy into the target
    bool applyMirror(Body3D* target, int plane, double offset, bool fuse);

    // Upload all bodies to GPU
    void uploadAllToGPU();

    // Undo/redo
    CommandManager3D& getCommandManager() { return commandManager; }
    bool undo() { return commandManager.undo(); }
    bool redo() { return commandManager.redo(); }

    // Scene stats
    int getBodyCount() const { return static_cast<int>(bodies.size()); }
    int getTotalFaces() const;
    int getTotalEdges() const;

private:
    std::vector<std::unique_ptr<Body3D>> bodies;
    std::vector<std::unique_ptr<Sketch3D>> sketches;
    Body3D* selectedBody{nullptr};
    Sketch3D* activeSketch{nullptr};
    WorkPlane3D activeWorkPlane;
    CommandManager3D commandManager;
};

} // namespace Modeling3D

