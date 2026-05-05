#include "modeling3d/Modeling3DScene.hpp"
#include <algorithm>
#include <iostream>

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
    if (profile.size() < 3) {
        std::cerr << "Sketch profile too small to extrude" << std::endl;
        return false;
    }

    Body3D* body = addBody();
    body->setName("Extrude " + std::to_string(body->getId()));
    bool ok = body->extrudeProfile(profile, direction, distance);
    if (!ok) {
        removeBody(body);
        return false;
    }

    body->uploadToGPU();
    selectBody(body);
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
        std::cerr << "Sketch profile too small to revolve" << std::endl;
        return false;
    }

    Body3D* body = addBody();
    body->setName("Revolve " + std::to_string(body->getId()));
    bool ok = body->revolveProfile(profile, axisOrigin, axisDir, angleDegrees);
    if (!ok) {
        removeBody(body);
        return false;
    }

    body->uploadToGPU();
    selectBody(body);
    finishSketch();
    return true;
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
