#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>

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

} // namespace Modeling3D
