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
    bool isSketchActive() const { return activeSketch != nullptr; }

    // Sketch storage
    const std::vector<std::unique_ptr<Sketch3D>>& getSketches() const { return sketches; }

    // Work planes
    const WorkPlane3D& getActiveWorkPlane() const { return activeWorkPlane; }
    void setActiveWorkPlane(const WorkPlane3D& wp) { activeWorkPlane = wp; }

    // Operations on selected body + active sketch
    bool extrudeActiveSketch(const glm::dvec3& direction, double distance);
    bool revolveActiveSketch(const glm::dvec3& axisOrigin,
                             const glm::dvec3& axisDir,
                             double angleDegrees);

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

