#include "modeling3d/CommandManager3D.hpp"
#include "modeling3d/Modeling3DScene.hpp"
#include "modeling3d/Body3D.hpp"

namespace Modeling3D {

// ─────────────────────────────────────────────
// CommandManager3D
// ─────────────────────────────────────────────

void CommandManager3D::execute(std::unique_ptr<Command3D> cmd) {
    // Truncate any redo history beyond cursor
    if (cursor < static_cast<int>(history.size())) {
        history.erase(history.begin() + cursor, history.end());
    }

    cmd->execute();
    history.push_back(std::move(cmd));
    cursor = static_cast<int>(history.size());
}

bool CommandManager3D::undo() {
    if (!canUndo()) return false;
    --cursor;
    history[cursor]->undo();
    return true;
}

bool CommandManager3D::redo() {
    if (!canRedo()) return false;
    history[cursor]->execute();
    ++cursor;
    return true;
}

std::string CommandManager3D::getUndoDescription() const {
    if (!canUndo()) return "";
    return history[cursor - 1]->getDescription();
}

std::string CommandManager3D::getRedoDescription() const {
    if (!canRedo()) return "";
    return history[cursor]->getDescription();
}

void CommandManager3D::clear() {
    history.clear();
    cursor = 0;
}

// ─────────────────────────────────────────────
// AddBodyCommand
// ─────────────────────────────────────────────

AddBodyCommand::AddBodyCommand(Modeling3DScene* scene, std::unique_ptr<Body3D> body, const std::string& desc)
    : scene(scene), body(std::move(body)), description(desc)
{
    bodyPtr = this->body.get();
}

void AddBodyCommand::execute() {
    if (!inScene && body) {
        scene->addExistingBody(std::move(body));
        inScene = true;
    }
}

void AddBodyCommand::undo() {
    if (inScene) {
        body = scene->detachBody(bodyPtr);
        inScene = false;
    }
}

// ─────────────────────────────────────────────
// DeleteBodyCommand
// ─────────────────────────────────────────────

DeleteBodyCommand::DeleteBodyCommand(Modeling3DScene* scene, Body3D* body)
    : scene(scene), bodyPtr(body)
{
    if (body) bodyName = body->getName();
}

void DeleteBodyCommand::execute() {
    if (inScene) {
        body = scene->detachBody(bodyPtr);
        inScene = false;
    }
}

void DeleteBodyCommand::undo() {
    if (!inScene && body) {
        scene->addExistingBody(std::move(body));
        inScene = true;
    }
}

// ─────────────────────────────────────────────
// TransformBodyCommand
// ─────────────────────────────────────────────

TransformBodyCommand::TransformBodyCommand(Body3D* body, const glm::mat4& oldTransform, const glm::mat4& newTransform)
    : body(body), oldTransform(oldTransform), newTransform(newTransform)
{
}

void TransformBodyCommand::execute() {
    if (body) body->setTransform(newTransform);
}

void TransformBodyCommand::undo() {
    if (body) body->setTransform(oldTransform);
}

// ─────────────────────────────────────────────
// ChangeColorCommand
// ─────────────────────────────────────────────

ChangeColorCommand::ChangeColorCommand(Body3D* body, const glm::vec3& oldColor, const glm::vec3& newColor)
    : body(body), oldColor(oldColor), newColor(newColor)
{
}

void ChangeColorCommand::execute() {
    if (body) body->setColor(newColor);
}

void ChangeColorCommand::undo() {
    if (body) body->setColor(oldColor);
}

} // namespace Modeling3D
