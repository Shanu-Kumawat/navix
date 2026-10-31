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

// ─────────────────────────────────────────────
// BooleanCommand
// ─────────────────────────────────────────────

BooleanCommand::BooleanCommand(Modeling3DScene* scene, Body3D* base, Body3D* tool,
                               BoolOp op, const std::string& desc)
    : scene(scene), basePtr(base), toolPtr(tool), op(op), description(desc)
{
}

BooleanCommand::~BooleanCommand() = default;

void BooleanCommand::execute() {
    if (applied) return;  // first run — work already performed by the scene

#ifdef USE_OCCT
    // Redo: detach the tool body again and re-apply the operation
    if (scene && basePtr && toolPtr) {
        detachedTool = scene->detachBody(toolPtr);
        if (detachedTool) {
            basePtr->applyBooleanFromWorldShapes(detachedTool->getWorldShape(), op);
            basePtr->uploadToGPU();
        }
    }
    applied = true;
#endif
}

void BooleanCommand::undo() {
#ifdef USE_OCCT
    if (!applied || !basePtr) return;

    // Restore the base body's original shape and transform
    basePtr->setOCCTShape(oldShape);
    basePtr->setTransform(oldTransform);
    basePtr->tessellateFromOCCT();
    basePtr->uploadToGPU();

    // Put the tool body back into the scene
    if (detachedTool && scene) {
        scene->addExistingBody(std::move(detachedTool));
    }
    applied = false;
#endif
}

// ─────────────────────────────────────────────
// PatternCommand
// ─────────────────────────────────────────────

PatternCommand::PatternCommand(Modeling3DScene* scene,
                               std::vector<std::unique_ptr<Body3D>> bodies,
                               const std::string& desc)
    : scene(scene), description(desc)
{
    for (auto& b : bodies) {
        bodyPtrs.push_back(b.get());
        ownedBodies.push_back(std::move(b));
    }
}

void PatternCommand::execute() {
    if (inScene) return;  // redo after undo
    for (auto& b : ownedBodies) {
        scene->addExistingBody(std::move(b));
    }
    ownedBodies.clear();
    inScene = true;
}

void PatternCommand::undo() {
    if (!inScene) return;
    for (Body3D* ptr : bodyPtrs) {
        ownedBodies.push_back(scene->detachBody(ptr));
    }
    inScene = false;
}

// ─────────────────────────────────────────────
// ModifyShapeCommand
// ─────────────────────────────────────────────

#ifdef USE_OCCT
ModifyShapeCommand::ModifyShapeCommand(Body3D* body, const TopoDS_Shape& oldShape,
                                       const glm::mat4& oldTransform,
                                       const TopoDS_Shape& newShape,
                                       const glm::mat4& newTransform,
                                       const std::string& desc)
    : body(body), oldShape(oldShape), oldTransform(oldTransform),
      newShape(newShape), newTransform(newTransform), description(desc)
{
}

void ModifyShapeCommand::apply(const TopoDS_Shape& shape, const glm::mat4& transform) {
    if (!body) return;
    body->setOCCTShape(shape);
    body->setTransform(transform);
    body->tessellateFromOCCT();
    body->uploadToGPU();
}

void ModifyShapeCommand::execute() {
    apply(newShape, newTransform);
}

void ModifyShapeCommand::undo() {
    apply(oldShape, oldTransform);
}
#endif

} // namespace Modeling3D
