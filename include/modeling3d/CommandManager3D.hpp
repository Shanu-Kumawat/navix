#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "modeling3d/Body3D.hpp"  // BoolOp
#ifdef USE_OCCT
#include <TopoDS_Shape.hxx>
#endif

namespace Modeling3D {

/**
 * @brief Abstract undo/redo command for the 3D modeling scene.
 */
class Command3D {
public:
    virtual ~Command3D() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;
};

/**
 * @brief Manages undo/redo history for 3D modeling operations.
 *
 * Follows the Command pattern. Each modifying operation creates a Command3D
 * that knows how to execute and undo itself. The manager maintains a linear
 * history stack with a cursor.
 */
class CommandManager3D {
public:
    CommandManager3D() = default;
    ~CommandManager3D() = default;

    // Execute a command and push it onto the history
    void execute(std::unique_ptr<Command3D> cmd);

    // Undo the last command (move cursor back)
    bool undo();

    // Redo the next command (move cursor forward)
    bool redo();

    // Check if undo/redo is available
    bool canUndo() const { return cursor > 0; }
    bool canRedo() const { return cursor < static_cast<int>(history.size()); }

    // Get description of what would be undone/redone
    std::string getUndoDescription() const;
    std::string getRedoDescription() const;

    // Clear all history
    void clear();

    // History size
    int getHistorySize() const { return static_cast<int>(history.size()); }
    int getCursor() const { return cursor; }

private:
    std::vector<std::unique_ptr<Command3D>> history;
    int cursor{0}; // Points to the next command to execute (i.e., past the last executed)
};

// ─────────────────────────────────────────────
// Concrete command implementations
// ─────────────────────────────────────────────

class Modeling3DScene;
class Body3D;

/**
 * @brief Command: Add a body to the scene (extrude/revolve result).
 * Undo removes the body; redo re-adds it.
 */
class AddBodyCommand : public Command3D {
public:
    AddBodyCommand(Modeling3DScene* scene, std::unique_ptr<Body3D> body, const std::string& desc);
    void execute() override;
    void undo() override;
    std::string getDescription() const override { return description; }

private:
    Modeling3DScene* scene;
    std::unique_ptr<Body3D> body;  // Owned when undone
    Body3D* bodyPtr{nullptr};      // Raw ptr when in scene
    std::string description;
    bool inScene{false};
};

/**
 * @brief Command: Delete a body from the scene.
 */
class DeleteBodyCommand : public Command3D {
public:
    DeleteBodyCommand(Modeling3DScene* scene, Body3D* body);
    void execute() override;
    void undo() override;
    std::string getDescription() const override { return "Delete " + bodyName; }

private:
    Modeling3DScene* scene;
    std::unique_ptr<Body3D> body;
    Body3D* bodyPtr{nullptr};
    std::string bodyName;
    bool inScene{true};
};

/**
 * @brief Command: Transform a body (move/rotate/scale).
 */
class TransformBodyCommand : public Command3D {
public:
    TransformBodyCommand(Body3D* body, const glm::mat4& oldTransform, const glm::mat4& newTransform);
    void execute() override;
    void undo() override;
    std::string getDescription() const override { return "Transform body"; }

private:
    Body3D* body;
    glm::mat4 oldTransform;
    glm::mat4 newTransform;
};

/**
 * @brief Command: Change body color.
 */
class ChangeColorCommand : public Command3D {
public:
    ChangeColorCommand(Body3D* body, const glm::vec3& oldColor, const glm::vec3& newColor);
    void execute() override;
    void undo() override;
    std::string getDescription() const override { return "Change color"; }

private:
    Body3D* body;
    glm::vec3 oldColor;
    glm::vec3 newColor;
};

/**
 * @brief Command: Boolean operation (union/subtract/intersect).
 *
 * The base body is modified in place and the tool body is removed from the
 * scene. Undo restores the base body's original B-Rep shape and re-adds the
 * tool body; redo re-applies the operation.
 */
class BooleanCommand : public Command3D {
public:
    BooleanCommand(Modeling3DScene* scene, Body3D* base, Body3D* tool,
                   BoolOp op, const std::string& desc);
    ~BooleanCommand() override;

    // Work is done by Modeling3DScene before pushing; execute() handles redo.
    void execute() override;
    void undo() override;
    std::string getDescription() const override { return description; }

    // Hand over ownership of the already-detached tool body
    void setDetachedTool(std::unique_ptr<Body3D> tool) { detachedTool = std::move(tool); }
#ifdef USE_OCCT
    // Snapshot for restoring the base body on undo
    void saveOldState(const TopoDS_Shape& shape, const glm::mat4& transform) {
        oldShape = shape; oldTransform = transform;
    }
#endif

private:
    Modeling3DScene* scene;
    Body3D* basePtr{nullptr};
    Body3D* toolPtr{nullptr};
    std::unique_ptr<Body3D> detachedTool;  // Owned when the op is applied
    BoolOp op;
    std::string description;
    bool applied{true};
    glm::mat4 oldTransform{1.0f};
#ifdef USE_OCCT
    TopoDS_Shape oldShape;
#endif
};

/**
 * @brief Command: add several bodies at once (linear/circular pattern).
 * Undo removes all copies; redo re-adds them in order.
 */
class PatternCommand : public Command3D {
public:
    PatternCommand(Modeling3DScene* scene,
                   std::vector<std::unique_ptr<Body3D>> bodies,
                   const std::string& desc);
    void execute() override;
    void undo() override;
    std::string getDescription() const override { return description; }

private:
    Modeling3DScene* scene;
    std::vector<Body3D*> bodyPtrs;
    std::vector<std::unique_ptr<Body3D>> ownedBodies;  // held when undone
    std::string description;
    bool inScene{false};
};

/**
 * @brief Command: merge (fuse) N tool bodies into a base body.
 * Undo restores the base shape and re-adds every tool; redo re-fuses.
 */
class MergeCommand : public Command3D {
public:
    MergeCommand(Modeling3DScene* scene, Body3D* base,
                 const std::string& desc);
    ~MergeCommand() override;

#ifdef USE_OCCT
    void saveOldState(const TopoDS_Shape& shape, const glm::mat4& transform) {
        oldShape = shape; oldTransform = transform;
    }
    void setTools(std::vector<Body3D*> toolPtrs,
                  std::vector<std::unique_ptr<Body3D>> detached);
    void setOp(BoolOp o) { op = o; }
#endif

    void execute() override;
    void undo() override;
    std::string getDescription() const override { return description; }

private:
    Modeling3DScene* scene;
    Body3D* basePtr{nullptr};
    std::vector<Body3D*> toolPtrs;
    std::vector<std::unique_ptr<Body3D>> ownedTools;  // held when applied
    std::string description;
    bool applied{true};
    BoolOp op{BoolOp::Union};
    glm::mat4 oldTransform{1.0f};
#ifdef USE_OCCT
    TopoDS_Shape oldShape;
#endif
};

/**
 * @brief Command: in-place shape modification (fillet, chamfer, ...).
 *
 * The operation is applied directly by the caller; this command stores the
 * old and new B-Rep shapes so undo/redo swaps them and re-tessellates.
 */
class ModifyShapeCommand : public Command3D {
public:
#ifdef USE_OCCT
    ModifyShapeCommand(Body3D* body, const TopoDS_Shape& oldShape,
                       const glm::mat4& oldTransform,
                       const TopoDS_Shape& newShape,
                       const glm::mat4& newTransform,
                       const std::string& desc);
#endif
    void execute() override;
    void undo() override;
    std::string getDescription() const override { return description; }

private:
    Body3D* body{nullptr};
    std::string description;
    glm::mat4 oldTransform{1.0f}, newTransform{1.0f};
#ifdef USE_OCCT
    TopoDS_Shape oldShape, newShape;
    void apply(const TopoDS_Shape& shape, const glm::mat4& transform);
#endif
};

} // namespace Modeling3D
