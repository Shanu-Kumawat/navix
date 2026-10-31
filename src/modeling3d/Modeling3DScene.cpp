#include "modeling3d/Modeling3DScene.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#ifdef USE_OCCT
#include <TopoDS_Shape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax2.hxx>
#include <gp_Trsf.hxx>
#include <glm/gtc/matrix_transform.hpp>

namespace {
int countFaces(const TopoDS_Shape& s) {
    int n = 0;
    if (s.IsNull()) return n;
    for (TopExp_Explorer e(s, TopAbs_FACE); e.More(); e.Next()) ++n;
    return n;
}
} // anonymous namespace
#endif

namespace Modeling3D {

Modeling3DScene::Modeling3DScene()
    : activeWorkPlane(WorkPlane3D::XY())
{
}

Body3D* Modeling3DScene::addBody() {
    auto body = std::make_unique<Body3D>();
    Body3D* ptr = body.get();
    bodies.push_back(std::move(body));
    return ptr;
}

void Modeling3DScene::removeBody(Body3D* body) {
    if (selectedBody == body) selectedBody = nullptr;
    auto it = std::find_if(bodies.begin(), bodies.end(),
        [body](const std::unique_ptr<Body3D>& b) { return b.get() == body; });
    if (it != bodies.end()) {
        bodies.erase(it);
    }
}

void Modeling3DScene::clearAll() {
    selectedBody = nullptr;
    activeSketch = nullptr;
    bodies.clear();
    sketches.clear();
    commandManager.clear();
}

void Modeling3DScene::addExistingBody(std::unique_ptr<Body3D> body) {
    if (!body) return;
    // Re-upload if needed
    if (!body->isUploaded()) {
        body->uploadToGPU();
    }
    bodies.push_back(std::move(body));
}

std::unique_ptr<Body3D> Modeling3DScene::detachBody(Body3D* body) {
    if (!body) return nullptr;
    if (selectedBody == body) selectedBody = nullptr;
    
    auto it = std::find_if(bodies.begin(), bodies.end(),
        [body](const std::unique_ptr<Body3D>& b) { return b.get() == body; });
    if (it != bodies.end()) {
        auto detached = std::move(*it);
        bodies.erase(it);
        return detached;
    }
    return nullptr;
}

Body3D* Modeling3DScene::getBodyById(uint64_t id) const {
    for (const auto& b : bodies) {
        if (b->getId() == id) return b.get();
    }
    return nullptr;
}

void Modeling3DScene::selectBody(Body3D* body) {
    // Deselect previous
    if (selectedBody) selectedBody->setSelected(false);
    selectedBody = body;
    if (selectedBody) selectedBody->setSelected(true);
}

void Modeling3DScene::clearSelection() {
    if (selectedBody) selectedBody->setSelected(false);
    selectedBody = nullptr;
}

void Modeling3DScene::beginSketch(const WorkPlane3D& plane) {
    if (activeSketch) finishSketch();
    auto sketch = std::make_unique<Sketch3D>(plane);
    sketch->setActive(true);
    sketch->setName("Sketch " + std::to_string(sketches.size() + 1));
    activeSketch = sketch.get();
    sketches.push_back(std::move(sketch));
    activeWorkPlane = plane;
}

void Modeling3DScene::finishSketch() {
    if (activeSketch) {
        activeSketch->setActive(false);
        activeSketch = nullptr;
    }
}

void Modeling3DScene::cancelSketch() {
    if (activeSketch) {
        // Remove the sketch entirely
        auto it = std::find_if(sketches.begin(), sketches.end(),
            [this](const std::unique_ptr<Sketch3D>& s) { return s.get() == activeSketch; });
        if (it != sketches.end()) {
            sketches.erase(it);
        }
        activeSketch = nullptr;
    }
}

bool Modeling3DScene::extrudeActiveSketch(const glm::dvec3& direction, double distance) {
    if (!activeSketch) {
        std::cerr << "No active sketch to extrude" << std::endl;
        return false;
    }

    auto profile = activeSketch->toWireProfile();
    if (profile.size() < 2) {
        std::cerr << "Sketch profile needs at least 2 points" << std::endl;
        return false;
    }

    auto body = std::make_unique<Body3D>();
    body->setName("Extrude " + std::to_string(body->getId()));
    body->setKind("Extrude");
    bool ok = body->extrudeProfile(profile, direction, distance);
    if (!ok) return false;

    body->uploadToGPU();
    Body3D* ptr = body.get();
    commandManager.execute(
        std::make_unique<AddBodyCommand>(this, std::move(body), "Extrude sketch"));
    selectBody(ptr);
    // The 2D profile has been consumed into the solid — hide it so the
    // flat wireframe doesn't stay overlaid on the 3D body
    if (activeSketch) activeSketch->setVisible(false);
    finishSketch();
    return true;
}


bool Modeling3DScene::revolveActiveSketch(const glm::dvec3& axisOrigin,
                                           const glm::dvec3& axisDir,
                                           double angleDegrees) {
    if (!activeSketch) {
        std::cerr << "No active sketch to revolve" << std::endl;
        return false;
    }

    auto profile = activeSketch->toWireProfile();
    if (profile.size() < 2) {
        std::cerr << "Sketch profile needs at least 2 points to revolve" << std::endl;
        return false;
    }

    auto body = std::make_unique<Body3D>();
    body->setName("Revolve " + std::to_string(body->getId()));
    body->setKind("Revolve");
    bool ok = body->revolveProfile(profile, axisOrigin, axisDir, angleDegrees);
    if (!ok) return false;

    body->uploadToGPU();
    Body3D* ptr = body.get();
    commandManager.execute(
        std::make_unique<AddBodyCommand>(this, std::move(body), "Revolve sketch"));
    selectBody(ptr);
    // The 2D profile has been consumed into the solid — hide it
    if (activeSketch) activeSketch->setVisible(false);
    finishSketch();
    return true;
}

bool Modeling3DScene::applyBoolean(Body3D* base, Body3D* tool, BoolOp op) {
    if (!base || !tool || base == tool) {
        std::cerr << "[Boolean] invalid selection: base=" << base << " tool=" << tool << std::endl;
        return false;
    }
    if (!base->hasOCCTShape() || !tool->hasOCCTShape()) {
        std::cerr << "[Boolean] missing B-Rep: base hasShape=" << base->hasOCCTShape()
                  << " tool hasShape=" << tool->hasOCCTShape() << std::endl;
        return false;
    }

#ifdef USE_OCCT
    // Snapshot the base body's state for undo
    TopoDS_Shape oldShape = base->getOCCTShape();
    glm::mat4 oldTransform = base->getTransform();

    if (!base->applyBooleanFromWorldShapes(tool->getWorldShape(), op)) return false;

    auto detached = detachBody(tool);
    base->uploadToGPU();
    selectBody(base);

    static const char* opNames[] = { "Union", "Subtract", "Intersect" };
    auto cmd = std::make_unique<BooleanCommand>(
        this, base, tool, op,
        std::string("Boolean ") + opNames[static_cast<int>(op)]);
    cmd->saveOldState(oldShape, oldTransform);
    cmd->setDetachedTool(std::move(detached));
    commandManager.execute(std::move(cmd));  // execute() is a no-op (already applied)
    return true;
#else
    (void)op;
    return false;
#endif
}

// ─────────────────────────────────────────────
// Cut-Extrude / Pocket
// ─────────────────────────────────────────────

bool Modeling3DScene::applyCutExtrude(Body3D* target, Sketch3D* sketch,
                                      const glm::dvec3& dir, double dist, double preShift) {
    if (!target || !sketch) return false;
#ifdef USE_OCCT
    if (!target->hasOCCTShape()) return false;

    auto profile = sketch->toWireProfile();
    if (profile.size() < 3) {
        std::cerr << "[CutExtrude] sketch profile needs at least 3 points" << std::endl;
        return false;
    }
    for (auto& p : profile) p -= dir * preShift;

    // Build the world-space cutter prism
    auto cutter = std::make_unique<Body3D>();
    if (!cutter->extrudeProfile(profile, dir, dist) || !cutter->hasOCCTShape()) {
        std::cerr << "[CutExtrude] cutter prism creation failed" << std::endl;
        return false;
    }

#ifdef USE_OCCT
    TopoDS_Shape oldShape = target->getOCCTShape();
    glm::mat4 oldTransform = target->getTransform();
#endif

    if (!target->applyBooleanFromWorldShapes(cutter->getWorldShape(), BoolOp::Subtract)) {
        std::cerr << "[CutExtrude] cut failed (no overlap?)" << std::endl;
        return false;
    }

    // Reject no-op cuts (cutter did not intersect the body — identical topology)
    if (countFaces(target->getOCCTShape()) == countFaces(oldShape)) {
        std::cerr << "[CutExtrude] cut removed no material" << std::endl;
        target->setOCCTShape(oldShape);
        target->setTransform(oldTransform);
        target->tessellateFromOCCT();
        return false;
    }

    target->uploadToGPU();
    selectBody(target);
    sketch->setVisible(false);  // profile consumed by the cut
    auto cmd = std::make_unique<ModifyShapeCommand>(
        target, oldShape, oldTransform,
        target->getOCCTShape(), target->getTransform(), "Cut-Extrude");
    commandManager.execute(std::move(cmd));
    return true;
#else
    (void)target; (void)sketch; (void)dir; (void)dist; (void)preShift;
    return false;
#endif
}

// ─────────────────────────────────────────────
// Hole (lite wizard)
// ─────────────────────────────────────────────

bool Modeling3DScene::applyHole(Body3D* target, const glm::dvec3& center,
                                const glm::dvec3& normal, HoleType type,
                                double dia, double depth,
                                double cbDia, double cbDepth) {
    if (!target || !target->hasOCCTShape()) return false;
#ifdef USE_OCCT
    glm::dvec3 inDir = glm::normalize(-normal);   // into the body
    glm::vec3 sz = target->getBoundsMax() - target->getBoundsMin();
    double diag = glm::length(sz);
    double through = 2.0 * diag + 10.0;

    TopoDS_Shape oldShape = target->getOCCTShape();
    glm::mat4 oldTransform = target->getTransform();

    // Build each cutter (world-space prism), subtract sequentially
    auto cutWith = [&](const glm::dvec3& c, double radius, double d) -> bool {
        auto cutter = std::make_unique<Body3D>();
        auto profile = Body3D::circleProfileWorld(c, inDir, radius);
        if (!cutter->extrudeProfile(profile, inDir, d) || !cutter->hasOCCTShape()) {
            std::cerr << "[Hole] cutter creation failed" << std::endl;
            return false;
        }
        return target->applyBooleanFromWorldShapes(cutter->getWorldShape(), BoolOp::Subtract);
    };

    bool ok = true;
    if (type == HoleType::Through) {
        ok = cutWith(center, dia * 0.5, through);
    } else if (type == HoleType::Blind) {
        ok = cutWith(center, dia * 0.5, depth);
    } else {  // Counterbore
        ok = cutWith(center, dia * 0.5, through);
        if (ok) ok = cutWith(center, cbDia * 0.5, cbDepth);
    }
    if (!ok) {
        std::cerr << "[Hole] cut failed" << std::endl;
        return false;
    }
    if (countFaces(target->getOCCTShape()) == countFaces(oldShape)) {
        std::cerr << "[Hole] hole removed no material" << std::endl;
        target->setOCCTShape(oldShape);
        target->setTransform(oldTransform);
        target->tessellateFromOCCT();
        return false;
    }

    target->uploadToGPU();
    selectBody(target);
    auto cmd = std::make_unique<ModifyShapeCommand>(
        target, oldShape, oldTransform,
        target->getOCCTShape(), target->getTransform(), "Hole");
    commandManager.execute(std::move(cmd));
    return true;
#else
    (void)target; (void)center; (void)normal; (void)type;
    (void)dia; (void)depth; (void)cbDia; (void)cbDepth;
    return false;
#endif
}

// ─────────────────────────────────────────────
// Linear / Circular pattern
// ─────────────────────────────────────────────

bool Modeling3DScene::applyPattern(Body3D* target, PatternKind kind,
                                   int linAxis, int linCount, double linSpacing,
                                   int circAxis, int circCount,
                                   const glm::dvec3& circCenterWorld) {
    if (!target || !target->hasOCCTShape()) return false;
    int count = (kind == PatternKind::Linear) ? linCount : circCount;
    if (count < 2) return false;

    std::vector<std::unique_ptr<Body3D>> clones;
    for (int i = 1; i < count; ++i) {
        glm::mat4 off(1.0f);
        if (kind == PatternKind::Linear) {
            glm::vec3 axes[3] = {{1,0,0},{0,1,0},{0,0,1}};
            off = glm::translate(glm::mat4(1.0f),
                                 axes[linAxis] * (float)(linSpacing * i));
        } else {
            glm::vec3 axes[3] = {{1,0,0},{0,1,0},{0,0,1}};
            glm::mat4 toC = glm::translate(glm::mat4(1.0f), glm::vec3(circCenterWorld));
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                        (2.0f * 3.14159265f * i) / count,
                                        axes[circAxis]);
            off = toC * rot * glm::inverse(toC);
        }

        auto clone = std::make_unique<Body3D>();
        clone->setOCCTShape(target->getOCCTShape());
        clone->setTransform(target->getTransform() * off);
        clone->setColor(target->getColor());
        clone->setName(target->getName() + " (" + std::to_string(i + 1) + ")");
        clone->setKind(target->getKind() == "Body" ? "Pattern" : target->getKind());
        clone->tessellateFromOCCT();
        clone->uploadToGPU();
        clones.push_back(std::move(clone));
    }

    static const char* kindNames[] = { "Linear pattern", "Circular pattern" };
    auto cmd = std::make_unique<PatternCommand>(this, std::move(clones),
                                                kindNames[(int)kind]);
    commandManager.execute(std::move(cmd));  // execute() adds the clones
    selectBody(target);
    return true;
}

// ─────────────────────────────────────────────
// Mirror
// ─────────────────────────────────────────────

bool Modeling3DScene::applyMirror(Body3D* target, int plane, double offset, bool fuse) {
    if (!target || !target->hasOCCTShape()) return false;
#ifdef USE_OCCT
    gp_Trsf m;
    if (plane == 0)      m.SetMirror(gp_Ax2(gp_Pnt(offset, 0, 0), gp_Dir(1, 0, 0)));
    else if (plane == 1) m.SetMirror(gp_Ax2(gp_Pnt(0, offset, 0), gp_Dir(0, 1, 0)));
    else                 m.SetMirror(gp_Ax2(gp_Pnt(0, 0, offset), gp_Dir(0, 0, 1)));

    BRepBuilderAPI_Transform xform(target->getWorldShape(), m, true /*copy*/);
    TopoDS_Shape mirrored = xform.Shape();
    if (mirrored.IsNull()) {
        std::cerr << "[Mirror] transform failed" << std::endl;
        return false;
    }

    if (fuse) {
        // Merge the mirrored copy into the target (symmetric solid)
        TopoDS_Shape oldShape = target->getOCCTShape();
        glm::mat4 oldTransform = target->getTransform();
        if (!target->applyBooleanFromWorldShapes(mirrored, BoolOp::Union)) {
            std::cerr << "[Mirror] fuse failed" << std::endl;
            return false;
        }
        target->uploadToGPU();
        selectBody(target);
        auto cmd = std::make_unique<ModifyShapeCommand>(
            target, oldShape, oldTransform,
            target->getOCCTShape(), target->getTransform(), "Mirror (fuse)");
        commandManager.execute(std::move(cmd));
    } else {
        // New standalone mirrored body
        auto mirroredBody = std::make_unique<Body3D>();
        mirroredBody->setOCCTShape(mirrored);
        mirroredBody->setColor(target->getColor());
        mirroredBody->setName("Mirror of " + target->getName());
        mirroredBody->setKind("Mirror");
        mirroredBody->tessellateFromOCCT();
        mirroredBody->uploadToGPU();
        Body3D* ptr = mirroredBody.get();
        commandManager.execute(std::make_unique<AddBodyCommand>(
            this, std::move(mirroredBody), ptr->getName()));
        selectBody(ptr);
    }
    return true;
#else
    (void)target; (void)plane; (void)offset; (void)fuse;
    return false;
#endif
}

void Modeling3DScene::uploadAllToGPU() {
    for (auto& body : bodies) {
        if (!body->isUploaded()) {
            body->uploadToGPU();
        }
    }
}

int Modeling3DScene::getTotalFaces() const {
    int count = 0;
    for (const auto& b : bodies) {
        count += static_cast<int>(b->getTriangles().size());
    }
    return count;
}

int Modeling3DScene::getTotalEdges() const {
    int count = 0;
    for (const auto& b : bodies) {
        count += static_cast<int>(b->getEdgeVertices().size()) / 2;
    }
    return count;
}

} // namespace Modeling3D
