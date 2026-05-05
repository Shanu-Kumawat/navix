#include "modeling3d/Viewport3D.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <imgui.h>

#ifdef USE_OCCT
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#endif

namespace Modeling3D {

Viewport3D::Viewport3D() {
}

Viewport3D::~Viewport3D() {
    if (framebuffer) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &colorTexture);
        glDeleteRenderbuffers(1, &depthRenderbuffer);
    }
    if (gridVAO)   { glDeleteVertexArrays(1, &gridVAO);   glDeleteBuffers(1, &gridVBO); }
    if (bgVAO)     { glDeleteVertexArrays(1, &bgVAO);     glDeleteBuffers(1, &bgVBO); }
    if (axisVAO)   { glDeleteVertexArrays(1, &axisVAO);   glDeleteBuffers(1, &axisVBO); }
    if (sketchVAO) { glDeleteVertexArrays(1, &sketchVAO); glDeleteBuffers(1, &sketchVBO); }
    if (previewVAO){ glDeleteVertexArrays(1, &previewVAO); glDeleteBuffers(1, &previewVBO); }
}

void Viewport3D::initialize() {
    if (initialized) return;

    camera = std::make_unique<Camera>(
        glm::vec3(25.0f, 20.0f, 25.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        -135.0f, -25.0f
    );
    camera->MovementSpeed = 10.0f;
    camera->MouseSensitivity = 0.15f;

    // Load shaders
    auto loadShader = [](const char* vs, const char* fs) -> std::unique_ptr<Shader> {
        try { return std::make_unique<Shader>(vs, fs); }
        catch (...) {
            std::string altVs = std::string("../") + vs;
            std::string altFs = std::string("../") + fs;
            try { return std::make_unique<Shader>(altVs.c_str(), altFs.c_str()); }
            catch (...) { return nullptr; }
        }
    };

    modelShader = loadShader("shaders/modeling3d.vs", "shaders/modeling3d.fs");
    gridShader  = loadShader("shaders/grid3d.vs", "shaders/grid3d.fs");
    bgShader    = loadShader("shaders/background3d.vs", "shaders/background3d.fs");

    if (!modelShader) {
        std::cerr << "Could not load 3D shaders" << std::endl;
        return;
    }

    // Fullscreen quad (shared by grid + background)
    float quad[] = {
        -1.0f, -1.0f,   1.0f, -1.0f,   1.0f,  1.0f,
        -1.0f, -1.0f,   1.0f,  1.0f,  -1.0f,  1.0f
    };

    // Background quad VAO (2D positions)
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Grid quad VAO (3D positions — z=0)
    float gridQuad[] = {
        -1.0f, -1.0f, 0.0f,   1.0f, -1.0f, 0.0f,   1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,   1.0f,  1.0f, 0.0f,  -1.0f,  1.0f, 0.0f
    };
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridQuad), gridQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Origin axis VAO — X(red) Y(green) Z(blue) lines from origin
    {
        // pos(3) + color(3) per vertex
        float axisLines[] = {
            // X axis — red
            0,0,0,  0.95f,0.22f,0.22f,
            30,0,0, 0.95f,0.22f,0.22f,
            // Y axis — green
            0,0,0,  0.22f,0.85f,0.30f,
            0,30,0, 0.22f,0.85f,0.30f,
            // Z axis — blue
            0,0,0,  0.22f,0.48f,0.95f,
            0,0,30, 0.22f,0.48f,0.95f,
        };
        glGenVertexArrays(1, &axisVAO);
        glGenBuffers(1, &axisVBO);
        glBindVertexArray(axisVAO);
        glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(axisLines), axisLines, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Note: color attr (location 1) not needed — we set objectColor per draw call
        glBindVertexArray(0);
    }

    initialized = true;
    std::cout << "Viewport3D initialized" << std::endl;
}

void Viewport3D::setupFramebuffer(int width, int height) {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Color texture
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    // Depth + stencil renderbuffer
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Viewport3D framebuffer not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    fbWidth = width;
    fbHeight = height;
}

void Viewport3D::resizeFramebuffer(int width, int height) {
    if (width == fbWidth && height == fbHeight) return;

    if (framebuffer) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &colorTexture);
        glDeleteRenderbuffers(1, &depthRenderbuffer);
        framebuffer = 0;
    }

    setupFramebuffer(width, height);
}

void Viewport3D::render(int width, int height) {
    if (!initialized) return;
    if (width <= 0 || height <= 0) return;

    // Ensure FBO is correct size
    if (!framebuffer || width != fbWidth || height != fbHeight) {
        resizeFramebuffer(width, height);
    }

    // Bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // === Gradient Background ===
    renderBackground();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CLIP_DISTANCE0);

    float aspect = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 1000.0f);
    glm::mat4 view = camera->GetViewMatrix();

    // NOTE: No horizontal floor grid — CAD tools don't render one.
    // Origin axes are drawn via the ImGui View Triad overlay instead.

    // === Bodies ===
    if (modelShader) {
        modelShader->use();
        modelShader->setMat4("projection", projection);
        modelShader->setMat4("view", view);
        modelShader->setVec3("viewPos", camera->Position);
        modelShader->setVec3("lightPos", lightPos);
        modelShader->setVec3("lightColor", lightColor);
        modelShader->setFloat("ambientStrength", 0.35f);
        modelShader->setFloat("diffuseStrength", 0.65f);
        modelShader->setFloat("specularStrength", 0.4f);
        modelShader->setFloat("shininess", 32.0f);
        modelShader->setVec4("clipPlane", glm::vec4(0));
        modelShader->setInt("renderMode", 0);

        for (const auto& body : scene.getBodies()) {
            if (!body->isVisible()) continue;
            glm::mat4 model = body->getTransform();
            modelShader->setMat4("model", model);
            modelShader->setVec3("objectColor", body->getColor());
            modelShader->setInt("isSelected", body->isSelected() ? 1 : 0);

            if (renderMode != RenderMode3D::Wireframe) {
                modelShader->setInt("renderMode", 0);
                body->draw();
            }
            if (renderMode == RenderMode3D::SolidWithEdges || renderMode == RenderMode3D::Wireframe) {
                modelShader->setInt("renderMode", 1);
                glLineWidth(1.5f);
                body->drawEdges();
            }
        }
    }

    // === All Sketches (active + finished) ===
    renderSketch();
    renderSketchPreview();
    renderExtrudePreview();

    // === World Origin Axes (X/Y/Z at center) ===
    renderOriginAxes();

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_CLIP_DISTANCE0);
}

void Viewport3D::renderGrid() {
    if (!gridShader) return;

    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 1000.0f);
    glm::mat4 view = camera->GetViewMatrix();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    gridShader->use();
    gridShader->setMat4("view", view);
    gridShader->setMat4("projection", projection);
    gridShader->setFloat("gridScale", 0.1f); // 10-unit grid spacing

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Viewport3D::rebuildSketchVAO() {
    Sketch3D* sketch = scene.getActiveSketch();
    if (!sketch) {
        sketchLineCount = 0;
        return;
    }

    const auto& shapes = sketch->getShapes();
    std::vector<glm::vec3> sketchLines;
    sketchLines.reserve(shapes.size() * 16); // reasonable pre-alloc

    for (const auto& shape : shapes) {
        if (shape->type == Drawing::ShapeType::LINE) {
            auto* line = static_cast<Drawing::Line*>(shape.get());
            sketchLines.push_back(glm::vec3(sketch->getWorkPlane().to3D(line->start)));
            sketchLines.push_back(glm::vec3(sketch->getWorkPlane().to3D(line->end)));
        }
        else if (shape->type == Drawing::ShapeType::CIRCLE) {
            auto* circle = static_cast<Drawing::Circle*>(shape.get());
            const int segments = 64;
            for (int i = 0; i < segments; ++i) {
                double a1 = (2.0 * M_PI * i) / segments;
                double a2 = (2.0 * M_PI * (i + 1)) / segments;
                glm::dvec2 p1(circle->center.x + circle->radius * cos(a1),
                              circle->center.y + circle->radius * sin(a1));
                glm::dvec2 p2(circle->center.x + circle->radius * cos(a2),
                              circle->center.y + circle->radius * sin(a2));
                sketchLines.push_back(glm::vec3(sketch->getWorkPlane().to3D(p1)));
                sketchLines.push_back(glm::vec3(sketch->getWorkPlane().to3D(p2)));
            }
        }
        else if (shape->type == Drawing::ShapeType::RECTANGLE) {
            auto* rect = static_cast<Drawing::Rectangle*>(shape.get());
            auto p0 = glm::vec3(sketch->getWorkPlane().to3D(rect->topLeft));
            auto p1 = glm::vec3(sketch->getWorkPlane().to3D(rect->topRight));
            auto p2 = glm::vec3(sketch->getWorkPlane().to3D(rect->bottomRight));
            auto p3 = glm::vec3(sketch->getWorkPlane().to3D(rect->bottomLeft));
            sketchLines.push_back(p0); sketchLines.push_back(p1);
            sketchLines.push_back(p1); sketchLines.push_back(p2);
            sketchLines.push_back(p2); sketchLines.push_back(p3);
            sketchLines.push_back(p3); sketchLines.push_back(p0);
        }
    }

    sketchLineCount = static_cast<int>(sketchLines.size());
    if (sketchLineCount == 0) return;

    // Create or reuse the VAO/VBO
    if (!sketchVAO) {
        glGenVertexArrays(1, &sketchVAO);
        glGenBuffers(1, &sketchVBO);
    }

    glBindVertexArray(sketchVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sketchVBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 sketchLines.size() * sizeof(glm::vec3),
                 sketchLines.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    sketchDirty = false;
}

void Viewport3D::renderSketch() {
    if (!modelShader) return;

    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 1000.0f);
    glm::mat4 view = camera->GetViewMatrix();

    modelShader->use();
    modelShader->setMat4("projection", projection);
    modelShader->setMat4("view", view);
    modelShader->setMat4("model", glm::mat4(1.0f));
    modelShader->setInt("renderMode", 1);
    modelShader->setInt("isSelected", 0);
    modelShader->setVec4("clipPlane", glm::vec4(0));
    glLineWidth(2.0f);

    // Render ALL stored sketches (finished and active) so geometry persists
    for (const auto& sk : scene.getSketches()) {
        if (!sk) continue;
        bool isActive = (sk.get() == scene.getActiveSketch());
        // Active sketch = bright cyan, finished = dimmer blue
        if (isActive)
            modelShader->setVec3("objectColor", glm::vec3(0.3f, 0.85f, 1.0f));
        else
            modelShader->setVec3("objectColor", glm::vec3(0.25f, 0.45f, 0.65f));

        // Build line geometry from this sketch on the fly
        std::vector<glm::vec3> lines;
        for (const auto& shape : sk->getShapes()) {
            if (shape->type == Drawing::ShapeType::LINE) {
                auto* ln = static_cast<Drawing::Line*>(shape.get());
                lines.push_back(glm::vec3(sk->getWorkPlane().to3D(ln->start)));
                lines.push_back(glm::vec3(sk->getWorkPlane().to3D(ln->end)));
            }
            else if (shape->type == Drawing::ShapeType::CIRCLE) {
                auto* c = static_cast<Drawing::Circle*>(shape.get());
                const int seg = 64;
                for (int i = 0; i < seg; ++i) {
                    double a1 = (2.0*M_PI*i)/seg, a2 = (2.0*M_PI*(i+1))/seg;
                    lines.push_back(glm::vec3(sk->getWorkPlane().to3D({c->center.x+c->radius*cos(a1), c->center.y+c->radius*sin(a1)})));
                    lines.push_back(glm::vec3(sk->getWorkPlane().to3D({c->center.x+c->radius*cos(a2), c->center.y+c->radius*sin(a2)})));
                }
            }
            else if (shape->type == Drawing::ShapeType::RECTANGLE) {
                auto* r = static_cast<Drawing::Rectangle*>(shape.get());
                auto p0 = glm::vec3(sk->getWorkPlane().to3D(r->topLeft));
                auto p1 = glm::vec3(sk->getWorkPlane().to3D(r->topRight));
                auto p2 = glm::vec3(sk->getWorkPlane().to3D(r->bottomRight));
                auto p3 = glm::vec3(sk->getWorkPlane().to3D(r->bottomLeft));
                lines.push_back(p0); lines.push_back(p1);
                lines.push_back(p1); lines.push_back(p2);
                lines.push_back(p2); lines.push_back(p3);
                lines.push_back(p3); lines.push_back(p0);
            }
        }
        if (lines.empty()) continue;

        // Upload and draw
        if (!sketchVAO) {
            glGenVertexArrays(1, &sketchVAO);
            glGenBuffers(1, &sketchVBO);
        }
        glBindVertexArray(sketchVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sketchVBO);
        glBufferData(GL_ARRAY_BUFFER, lines.size()*sizeof(glm::vec3), lines.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
        glBindVertexArray(0);
    }
}

void Viewport3D::renderBackground() {
    if (!bgShader) return;
    glDisable(GL_DEPTH_TEST);
    bgShader->use();
    glBindVertexArray(bgVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Viewport3D::renderWorkPlaneHighlight() {
    // Intentionally empty — visible plane rectangle is distracting in CAD.
}

void Viewport3D::renderOriginAxes() {
    if (!modelShader || !axisVAO) return;

    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 proj = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 1000.0f);
    glm::mat4 view = camera->GetViewMatrix();

    modelShader->use();
    modelShader->setMat4("projection", proj);
    modelShader->setMat4("view", view);
    modelShader->setMat4("model", glm::mat4(1.0f));
    modelShader->setInt("renderMode", 1);    // line/wireframe mode
    modelShader->setInt("isSelected", 0);
    modelShader->setVec4("clipPlane", glm::vec4(0));
    modelShader->setFloat("ambientStrength", 1.0f);  // full bright
    modelShader->setFloat("diffuseStrength", 0.0f);
    modelShader->setFloat("specularStrength", 0.0f);

    // Draw through everything so axes are always visible
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.5f);
    glBindVertexArray(axisVAO);

    // X axis — red
    modelShader->setVec3("objectColor", glm::vec3(0.95f, 0.22f, 0.22f));
    glDrawArrays(GL_LINES, 0, 2);

    // Y axis — green
    modelShader->setVec3("objectColor", glm::vec3(0.22f, 0.85f, 0.30f));
    glDrawArrays(GL_LINES, 2, 2);

    // Z axis — blue
    modelShader->setVec3("objectColor", glm::vec3(0.22f, 0.48f, 0.95f));
    glDrawArrays(GL_LINES, 4, 2);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);

    // Restore lighting defaults
    modelShader->setFloat("ambientStrength", 0.35f);
    modelShader->setFloat("diffuseStrength", 0.65f);
    modelShader->setFloat("specularStrength", 0.4f);
}

// ─────────────────────────────────────────────
// Input handling
// ─────────────────────────────────────────────

void Viewport3D::handleMouseButton(int button, bool pressed, float x, float y) {
    lastMouseX = x;
    lastMouseY = y;

    if (button == 1) { // Middle
        middleMouseDown = pressed;
    }
    else if (button == 0) { // Left
        leftMouseDown = pressed;
        if (pressed) {
            if (scene.isSketchActive() && sketchDrawTool != SketchDrawTool::None) {
                handleSketchClick(x, y);
            } else if (activeTool == Tool3DType::Select) {
                Body3D* hit = pickBody(x, y);
                scene.selectBody(hit);
            }
        }
    }
    else if (button == 2) { // Right
        rightMouseDown = pressed;
        if (pressed && sketchHasFirstPoint) {
            sketchHasFirstPoint = false;
        }
    }
}

void Viewport3D::handleMouseMove(float x, float y) {
    float dx = x - lastMouseX;
    float dy = y - lastMouseY;

    // MMB drag — orbit (default) / Pan (Shift) / Zoom (Ctrl)
    if (middleMouseDown) {
        if (shiftDown) {
            float panSpeed = 0.05f;
            camera->Position += camera->Right * (-dx * panSpeed) + camera->Up * (dy * panSpeed);
        } else if (ctrlDown) {
            camera->Position += camera->Front * (-dy * 0.15f);
        } else {
            camera->ProcessMouseMovement(dx, -dy);
        }
    }

    // Right-drag:
    //   - While sketching: right-click ONLY cancels the current point (handled in handleMouseButton).
    //     Do NOT pan/orbit on right-drag during sketching to avoid accidental view changes.
    //   - Outside sketch: right-drag = orbit (most CAD tools use RMB to orbit).
    if (rightMouseDown) {
        if (!scene.isSketchActive()) {
            // Orbit on right-drag (like CATIA/SpaceClaim)
            camera->ProcessMouseMovement(dx * 0.7f, -dy * 0.7f);
        }
        // Inside sketch: do nothing on drag — RMB is only for click-cancel
    }

    // Sketch cursor tracking
    if (scene.isSketchActive() && sketchDrawTool != SketchDrawTool::None) {
        handleSketchMove(x, y);
    }

    lastMouseX = x;
    lastMouseY = y;
}

void Viewport3D::handleMouseScroll(float yOffset) {
    float zoomSpeed = 2.0f;
    camera->Position += camera->Front * yOffset * zoomSpeed;
}

void Viewport3D::handleKey(int key, bool pressed) {
    if (key == 340 || key == 344) shiftDown = pressed;   // Shift
    if (key == 341 || key == 345) ctrlDown = pressed;    // Ctrl
}

void Viewport3D::resetCamera() {
    if (camera) {
        camera->Position = glm::vec3(25.0f, 20.0f, 25.0f);
        camera->Yaw = -135.0f;
        camera->Pitch = -25.0f;
        camera->ProcessMouseMovement(0, 0); // Recalculate vectors
    }
}

void Viewport3D::setWorkPlane(const WorkPlane3D& wp) {
    scene.setActiveWorkPlane(wp);
}

bool Viewport3D::executeExtrude() {
    if (!scene.isSketchActive()) return false;
    glm::dvec3 dir = scene.getActiveWorkPlane().getNormal();
    return scene.extrudeActiveSketch(dir, extrudeDistance);
}

bool Viewport3D::executeRevolve() {
    if (!scene.isSketchActive()) return false;
    const WorkPlane3D& wp = scene.getActiveWorkPlane();
    glm::dvec3 axisOrigin = wp.getOrigin();
    glm::dvec3 axisDir;
    switch (revolveAxisMode) {
        case RevolveAxis::SketchX:  axisDir = wp.getXDirection(); break;
        case RevolveAxis::SketchY:  axisDir = wp.getYDirection(); break;
        case RevolveAxis::GlobalX:  axisDir = {1,0,0}; axisOrigin = {0,0,0}; break;
        case RevolveAxis::GlobalY:  axisDir = {0,1,0}; axisOrigin = {0,0,0}; break;
        case RevolveAxis::GlobalZ:  axisDir = {0,0,1}; axisOrigin = {0,0,0}; break;
        default:                    axisDir = wp.getYDirection(); break;
    }
    return scene.revolveActiveSketch(axisOrigin, axisDir, revolveAngle);
}

glm::dvec3 Viewport3D::screenToRay(float screenX, float screenY) const {
    if (!camera || fbWidth <= 0 || fbHeight <= 0) return glm::dvec3(0, 0, -1);

    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 1000.0f);
    glm::mat4 view = camera->GetViewMatrix();

    // NDC
    float nx = (2.0f * screenX / fbWidth) - 1.0f;
    float ny = 1.0f - (2.0f * screenY / fbHeight);

    glm::vec4 rayClip(nx, ny, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    return glm::dvec3(rayWorld);
}

Body3D* Viewport3D::pickBody(float screenX, float screenY) const {
    glm::dvec3 rayDir = screenToRay(screenX, screenY);
    glm::dvec3 rayOrigin(camera->Position);

    Body3D* closest = nullptr;
    double closestDist = std::numeric_limits<double>::max();

    for (const auto& body : scene.getBodies()) {
        if (!body->isVisible()) continue;

        // Proper slab-based AABB ray intersection
        glm::vec3 bmin = body->getBoundsMin();
        glm::vec3 bmax = body->getBoundsMax();

        // Transform bounds by body's model matrix
        glm::mat4 xform = body->getTransform();
        glm::vec3 corners[8] = {
            glm::vec3(xform * glm::vec4(bmin.x, bmin.y, bmin.z, 1)),
            glm::vec3(xform * glm::vec4(bmax.x, bmin.y, bmin.z, 1)),
            glm::vec3(xform * glm::vec4(bmin.x, bmax.y, bmin.z, 1)),
            glm::vec3(xform * glm::vec4(bmax.x, bmax.y, bmin.z, 1)),
            glm::vec3(xform * glm::vec4(bmin.x, bmin.y, bmax.z, 1)),
            glm::vec3(xform * glm::vec4(bmax.x, bmin.y, bmax.z, 1)),
            glm::vec3(xform * glm::vec4(bmin.x, bmax.y, bmax.z, 1)),
            glm::vec3(xform * glm::vec4(bmax.x, bmax.y, bmax.z, 1))
        };

        // Compute world-space AABB from transformed corners
        glm::vec3 wmin = corners[0], wmax = corners[0];
        for (int i = 1; i < 8; ++i) {
            wmin = glm::min(wmin, corners[i]);
            wmax = glm::max(wmax, corners[i]);
        }

        // Ray-AABB intersection (slab method)
        double tMin = 0.0, tMax = std::numeric_limits<double>::max();
        for (int axis = 0; axis < 3; ++axis) {
            double invD = 1.0 / rayDir[axis];
            double t0 = (static_cast<double>(wmin[axis]) - rayOrigin[axis]) * invD;
            double t1 = (static_cast<double>(wmax[axis]) - rayOrigin[axis]) * invD;
            if (invD < 0.0) std::swap(t0, t1);
            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);
            if (tMax < tMin) break;
        }

        if (tMax >= tMin && tMin < closestDist) {
            closestDist = tMin;
            closest = body.get();
        }
    }

    return closest;
}

std::string Viewport3D::getStatusText() const {
    std::string text;
    switch (activeTool) {
        case Tool3DType::Select:  text = "Select"; break;
        case Tool3DType::Sketch:  text = "Sketch"; break;
        case Tool3DType::Extrude: text = "Extrude"; break;
        case Tool3DType::Revolve: text = "Revolve"; break;
        case Tool3DType::Fillet:  text = "Fillet"; break;
        case Tool3DType::Move:    text = "Move"; break;
        case Tool3DType::Rotate:  text = "Rotate"; break;
        case Tool3DType::Scale:   text = "Scale"; break;
    }

    if (scene.isSketchActive()) {
        text += " | Sketch Active on " + scene.getActiveWorkPlane().getName();
    }

    text += " | Bodies: " + std::to_string(scene.getBodyCount());
    text += " | Faces: " + std::to_string(scene.getTotalFaces());

    return text;
}

glm::mat4 Viewport3D::getGizmoMatrix() const {
    if (scene.getSelectedBody()) {
        return scene.getSelectedBody()->getTransform();
    }
    return glm::mat4(1.0f);
}

void Viewport3D::setGizmoMatrix(const glm::mat4& m) {
    if (scene.getSelectedBody()) {
        scene.getSelectedBody()->setTransform(m);
    }
}

// ─────────────────────────────────────────────
// Screen-to-WorkPlane projection
// ─────────────────────────────────────────────

glm::dvec2 Viewport3D::screenToWorkPlane(float screenX, float screenY) const {
    if (!scene.isSketchActive()) return sketchCursor2D; // Return last known

    glm::dvec3 rayDir = screenToRay(screenX, screenY);
    glm::dvec3 rayOrigin(camera->Position);

    const WorkPlane3D& wp = scene.getActiveWorkPlane();

    // Check for near-parallel ray (dot product with plane normal near zero)
    double denom = glm::dot(rayDir, wp.getNormal());
    if (std::abs(denom) < 1e-6) return sketchCursor2D; // Ray nearly parallel

    double t = wp.intersectRay(rayOrigin, rayDir);
    if (t < 0 || t > 1e6) return sketchCursor2D; // Behind camera or too far

    glm::dvec3 hitPoint = rayOrigin + rayDir * t;
    return wp.to2D(hitPoint);
}

// ─────────────────────────────────────────────
// World-to-Screen Projection
// ─────────────────────────────────────────────

glm::dvec2 Viewport3D::worldToScreen(const glm::dvec3& worldPt) const {
    if (!camera || fbWidth <= 0 || fbHeight <= 0) return {-1,-1};
    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 proj = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 1000.0f);
    glm::mat4 view = camera->GetViewMatrix();
    glm::vec4 clip = proj * view * glm::vec4(glm::vec3(worldPt), 1.0f);
    if (std::abs(clip.w) < 1e-6f) return {-1,-1};
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    // Returns position in FBO pixel space
    return glm::dvec2(
        (ndc.x + 1.0f) * 0.5f * fbWidth,
        (1.0f - ndc.y) * 0.5f * fbHeight
    );
}

// ─────────────────────────────────────────────
// Geometric Snap
// ─────────────────────────────────────────────

glm::dvec2 Viewport3D::findGeometricSnap(float mouseX, float mouseY,
                                          glm::dvec2 rawPt,
                                          SnapType& outType) const {
    if (!scene.isSketchActive()) { outType = SnapType::None; return rawPt; }

    const WorkPlane3D& wp = scene.getActiveWorkPlane();
    const float SNAP_PX = 18.0f; // pixel radius for geometric snap

    struct Cand { glm::dvec2 pt2d; SnapType type; int priority; };
    std::vector<Cand> cands;

    // World origin (projected onto current work plane)
    cands.push_back({ wp.to2D(glm::dvec3(0,0,0)), SnapType::Origin, 0 });

    // Snap to start-of-chain (current first point if drawing)
    if (sketchHasFirstPoint)
        cands.push_back({ sketchFirstPoint, SnapType::Endpoint, 0 });

    // All shapes in all sketches
    for (const auto& sk : scene.getSketches()) {
        if (!sk) continue;
        for (const auto& shape : sk->getShapes()) {
            using T = Drawing::ShapeType;
            if (shape->type == T::LINE) {
                auto* ln = static_cast<Drawing::Line*>(shape.get());
                // Project line endpoints onto current work plane via 3D
                cands.push_back({ wp.to2D(sk->getWorkPlane().to3D(ln->start)), SnapType::Endpoint, 1 });
                cands.push_back({ wp.to2D(sk->getWorkPlane().to3D(ln->end)),   SnapType::Endpoint, 1 });
                glm::dvec2 mid2d = (ln->start + ln->end) * 0.5;
                cands.push_back({ wp.to2D(sk->getWorkPlane().to3D(mid2d)),     SnapType::Midpoint, 2 });
            }
            else if (shape->type == T::CIRCLE) {
                auto* c = static_cast<Drawing::Circle*>(shape.get());
                cands.push_back({ wp.to2D(sk->getWorkPlane().to3D(c->center)), SnapType::Center, 1 });
            }
            else if (shape->type == T::RECTANGLE) {
                auto* r = static_cast<Drawing::Rectangle*>(shape.get());
                for (auto& corner : {r->topLeft, r->topRight, r->bottomLeft, r->bottomRight})
                    cands.push_back({ wp.to2D(sk->getWorkPlane().to3D(corner)), SnapType::Endpoint, 1 });
            }
        }
    }

    // Find closest within SNAP_PX
    float bestDist = SNAP_PX;
    int bestPriority = 999;
    glm::dvec2 bestPt = rawPt;
    SnapType bestType = SnapType::None;

    for (const auto& c : cands) {
        glm::dvec3 w3d = wp.to3D(c.pt2d);
        glm::dvec2 screen = worldToScreen(w3d);
        float dist = (float)glm::length(glm::dvec2(mouseX, mouseY) - screen);
        bool closer = dist < bestDist;
        bool sameDist = dist < bestDist + 3.0f && c.priority < bestPriority;
        if (closer || sameDist) {
            bestDist = dist;
            bestPt = c.pt2d;
            bestType = c.type;
            bestPriority = c.priority;
        }
    }

    outType = bestType;
    return bestPt;
}

// ─────────────────────────────────────────────
// Interactive Sketch Drawing
// ─────────────────────────────────────────────

void Viewport3D::handleSketchClick(float x, float y) {
    if (!scene.isSketchActive()) return;

    glm::dvec2 raw = screenToWorkPlane(x, y);

    // 1. Geometric snap (highest priority)
    SnapType geoType = SnapType::None;
    glm::dvec2 pt = findGeometricSnap(x, y, raw, geoType);

    // 2. Grid snap fallback (if no geometric snap and snap enabled)
    if (geoType == SnapType::None && snapEnabled && !ctrlDown) {
        pt.x = std::round(pt.x / snapGridSize) * snapGridSize;
        pt.y = std::round(pt.y / snapGridSize) * snapGridSize;
        currentSnapType = SnapType::Grid;
    } else {
        currentSnapType = geoType;
    }
    snapPoint2D = pt;
    sketchCursor2D = pt;

    if (!sketchHasFirstPoint) {
        sketchFirstPoint = pt;
        sketchHasFirstPoint = true;
    } else {
        commitSketchShape();
        // Chain mode: for Line tool, keep first point = last placed point
        if (sketchChainMode && sketchDrawTool == SketchDrawTool::Line) {
            sketchFirstPoint = pt;
            // sketchHasFirstPoint stays true — continues chain
        } else {
            sketchHasFirstPoint = false;
        }
    }
}

void Viewport3D::handleSketchMove(float x, float y) {
    glm::dvec2 raw = screenToWorkPlane(x, y);

    // 1. Geometric snap
    SnapType geoType = SnapType::None;
    glm::dvec2 snapped = findGeometricSnap(x, y, raw, geoType);

    // 2. Grid snap fallback
    if (geoType == SnapType::None && snapEnabled && !ctrlDown) {
        snapped.x = std::round(snapped.x / snapGridSize) * snapGridSize;
        snapped.y = std::round(snapped.y / snapGridSize) * snapGridSize;
        currentSnapType = SnapType::Grid;
    } else {
        currentSnapType = geoType;
    }

    sketchCursor2D = snapped;
    snapPoint2D    = snapped;
    snapIsActive   = (currentSnapType != SnapType::None);
}

void Viewport3D::commitSketchShape() {
    if (!scene.isSketchActive()) return;
    auto* sketch = scene.getActiveSketch();
    if (!sketch) return;

    glm::dvec2 p1 = sketchFirstPoint;
    glm::dvec2 p2 = sketchCursor2D;

    switch (sketchDrawTool) {
        case SketchDrawTool::Line: {
            if (glm::length(p2 - p1) > 0.001) {
                auto line = std::make_unique<Drawing::Line>(p1, p2);
                sketch->addShape(std::move(line));
            }
            break;
        }
        case SketchDrawTool::Rectangle: {
            glm::dvec2 tl(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
            glm::dvec2 br(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
            if (glm::length(br - tl) > 0.001) {
                auto rect = std::make_unique<Drawing::Rectangle>(
                    glm::dvec2(tl.x, tl.y), glm::dvec2(br.x, tl.y),
                    glm::dvec2(br.x, br.y), glm::dvec2(tl.x, br.y));
                sketch->addShape(std::move(rect));
            }
            break;
        }
        case SketchDrawTool::Circle: {
            double radius = glm::length(p2 - p1);
            if (radius > 0.01) {
                auto circle = std::make_unique<Drawing::Circle>(p1, radius);
                sketch->addShape(std::move(circle));
            }
            break;
        }
        case SketchDrawTool::Spline: {
            // Store as a 2-point spline (user builds up by clicking)
            if (glm::length(p2 - p1) > 0.001) {
                auto line = std::make_unique<Drawing::Line>(p1, p2);
                sketch->addShape(std::move(line));
            }
            break;
        }
        default:
            break;
    }

    sketchDirty = true;
}

// ─────────────────────────────────────────────
// Sketch Preview (rubber-band lines)
// ─────────────────────────────────────────────

void Viewport3D::renderSketchPreview() {
    if (!scene.isSketchActive() || !sketchHasFirstPoint || !modelShader) return;

    const WorkPlane3D& wp = scene.getActiveWorkPlane();
    std::vector<glm::vec3> lines;

    glm::dvec2 p1 = sketchFirstPoint;
    glm::dvec2 p2 = sketchCursor2D;

    switch (sketchDrawTool) {
        case SketchDrawTool::Line:
            lines.push_back(glm::vec3(wp.to3D(p1)));
            lines.push_back(glm::vec3(wp.to3D(p2)));
            break;

        case SketchDrawTool::Rectangle: {
            glm::dvec2 tl(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
            glm::dvec2 br(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
            auto c0 = glm::vec3(wp.to3D(glm::dvec2(tl.x, tl.y)));
            auto c1 = glm::vec3(wp.to3D(glm::dvec2(br.x, tl.y)));
            auto c2 = glm::vec3(wp.to3D(glm::dvec2(br.x, br.y)));
            auto c3 = glm::vec3(wp.to3D(glm::dvec2(tl.x, br.y)));
            lines.push_back(c0); lines.push_back(c1);
            lines.push_back(c1); lines.push_back(c2);
            lines.push_back(c2); lines.push_back(c3);
            lines.push_back(c3); lines.push_back(c0);
            break;
        }

        case SketchDrawTool::Circle: {
            double radius = glm::length(p2 - p1);
            if (radius > 0.01) {
                const int segs = 64;
                for (int i = 0; i < segs; ++i) {
                    double a1 = (2.0 * M_PI * i) / segs;
                    double a2 = (2.0 * M_PI * (i + 1)) / segs;
                    glm::dvec2 cp1(p1.x + radius * cos(a1), p1.y + radius * sin(a1));
                    glm::dvec2 cp2(p1.x + radius * cos(a2), p1.y + radius * sin(a2));
                    lines.push_back(glm::vec3(wp.to3D(cp1)));
                    lines.push_back(glm::vec3(wp.to3D(cp2)));
                }
            }
            break;
        }

        default:
            break;
    }

    if (lines.empty()) return;

    // Upload preview lines to a temp VAO
    if (!previewVAO) {
        glGenVertexArrays(1, &previewVAO);
        glGenBuffers(1, &previewVBO);
    }
    glBindVertexArray(previewVAO);
    glBindBuffer(GL_ARRAY_BUFFER, previewVBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3),
                 lines.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 proj = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 1000.0f);
    glm::mat4 view = camera->GetViewMatrix();

    modelShader->use();
    modelShader->setMat4("projection", proj);
    modelShader->setMat4("view", view);
    modelShader->setMat4("model", glm::mat4(1.0f));
    modelShader->setVec3("objectColor", glm::vec3(1.0f, 0.6f, 0.1f)); // Orange preview
    modelShader->setInt("renderMode", 1);
    modelShader->setInt("isSelected", 0);
    modelShader->setVec4("clipPlane", glm::vec4(0));

    glLineWidth(2.5f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
    glBindVertexArray(0);
}

void Viewport3D::renderExtrudePreview() {
    // Only show extrude ghost when tool is active and sketch exists
    if (activeTool != Tool3DType::Extrude) return;
    if (!scene.isSketchActive() || !modelShader) return;

    Sketch3D* sketch = scene.getActiveSketch();
    if (!sketch) return;
    auto profile = sketch->toWireProfile();
    if (profile.empty()) return;

    const WorkPlane3D& wp = scene.getActiveWorkPlane();
    glm::dvec3 dir = wp.getNormal() * (double)extrudeDistance;

    // Build ghost wireframe: top profile + side edge lines
    std::vector<glm::vec3> lines;
    int n = static_cast<int>(profile.size());

    // Bottom profile edges
    for (int i = 0; i < n; ++i) {
        lines.push_back(glm::vec3(profile[i]));
        lines.push_back(glm::vec3(profile[(i + 1) % n]));
    }
    // Top profile edges (offset by extrude distance)
    for (int i = 0; i < n; ++i) {
        lines.push_back(glm::vec3(profile[i] + dir));
        lines.push_back(glm::vec3(profile[(i + 1) % n] + dir));
    }
    // Vertical side edges
    for (int i = 0; i < n; ++i) {
        lines.push_back(glm::vec3(profile[i]));
        lines.push_back(glm::vec3(profile[i] + dir));
    }

    if (lines.empty()) return;

    if (!previewVAO) {
        glGenVertexArrays(1, &previewVAO);
        glGenBuffers(1, &previewVBO);
    }
    glBindVertexArray(previewVAO);
    glBindBuffer(GL_ARRAY_BUFFER, previewVBO);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3),
                 lines.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 proj = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 1000.0f);
    glm::mat4 view = camera->GetViewMatrix();

    modelShader->use();
    modelShader->setMat4("projection", proj);
    modelShader->setMat4("view", view);
    modelShader->setMat4("model", glm::mat4(1.0f));
    modelShader->setVec3("objectColor", glm::vec3(0.2f, 0.9f, 0.4f)); // Green ghost
    modelShader->setInt("renderMode", 1);
    modelShader->setInt("isSelected", 0);
    modelShader->setVec4("clipPlane", glm::vec4(0));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────
// OCCT Primitive Creation
// ─────────────────────────────────────────────

Body3D* Viewport3D::createBox(double dx, double dy, double dz) {
    auto body = std::make_unique<Body3D>();
    body->setName("Box");

#ifdef USE_OCCT
    try {
        BRepPrimAPI_MakeBox box(dx, dy, dz);
        box.Build();
        if (box.IsDone()) {
            body->setOCCTShape(box.Shape());
        }
    } catch (...) {}
#endif

    // Fallback: build a simple box mesh if no OCCT
    if (body->getVertices().empty()) {
        std::vector<glm::dvec3> profile = {
            {0, 0, 0}, {dx, 0, 0}, {dx, dy, 0}, {0, dy, 0}
        };
        body->extrudeProfile(profile, {0, 0, 1}, dz);
    }

    body->uploadToGPU();
    Body3D* ptr = body.get();
    scene.addExistingBody(std::move(body));
    return ptr;
}

Body3D* Viewport3D::createCylinder(double radius, double height) {
    auto body = std::make_unique<Body3D>();
    body->setName("Cylinder");

#ifdef USE_OCCT
    try {
        BRepPrimAPI_MakeCylinder cyl(radius, height);
        cyl.Build();
        if (cyl.IsDone()) {
            body->setOCCTShape(cyl.Shape());
        }
    } catch (...) {}
#endif

    if (body->getVertices().empty()) {
        // Fallback: circular profile extruded
        const int segs = 64;
        std::vector<glm::dvec3> profile;
        for (int i = 0; i < segs; ++i) {
            double a = (2.0 * M_PI * i) / segs;
            profile.push_back({radius * cos(a), radius * sin(a), 0});
        }
        body->extrudeProfile(profile, {0, 0, 1}, height);
    }

    body->uploadToGPU();
    Body3D* ptr = body.get();
    scene.addExistingBody(std::move(body));
    return ptr;
}

Body3D* Viewport3D::createSphere(double radius) {
    auto body = std::make_unique<Body3D>();
    body->setName("Sphere");

#ifdef USE_OCCT
    try {
        BRepPrimAPI_MakeSphere sph(radius);
        sph.Build();
        if (sph.IsDone()) {
            body->setOCCTShape(sph.Shape());
        }
    } catch (...) {}
#endif

    if (body->getVertices().empty()) {
        // Fallback: revolve a half-circle
        const int segs = 32;
        std::vector<glm::dvec3> profile;
        for (int i = 0; i <= segs; ++i) {
            double a = (M_PI * i) / segs;
            profile.push_back({radius * sin(a), 0, radius * cos(a)});
        }
        body->revolveProfile(profile, {0, 0, 0}, {0, 0, 1}, 360.0);
    }

    body->uploadToGPU();
    Body3D* ptr = body.get();
    scene.addExistingBody(std::move(body));
    return ptr;
}

Body3D* Viewport3D::createCone(double r1, double r2, double height) {
    auto body = std::make_unique<Body3D>();
    body->setName("Cone");

#ifdef USE_OCCT
    try {
        BRepPrimAPI_MakeCone cone(r1, r2, height);
        cone.Build();
        if (cone.IsDone()) {
            body->setOCCTShape(cone.Shape());
        }
    } catch (...) {}
#endif

    if (body->getVertices().empty()) {
        // Fallback: revolve a trapezoid profile
        std::vector<glm::dvec3> profile = {
            {r1, 0, 0}, {r2, 0, height}, {0, 0, height}, {0, 0, 0}
        };
        body->revolveProfile(profile, {0, 0, 0}, {0, 0, 1}, 360.0);
    }

    body->uploadToGPU();
    Body3D* ptr = body.get();
    scene.addExistingBody(std::move(body));
    return ptr;
}

Body3D* Viewport3D::createTorus(double majorR, double minorR) {
    auto body = std::make_unique<Body3D>();
    body->setName("Torus");

#ifdef USE_OCCT
    try {
        BRepPrimAPI_MakeTorus torus(majorR, minorR);
        torus.Build();
        if (torus.IsDone()) {
            body->setOCCTShape(torus.Shape());
        }
    } catch (...) {}
#endif

    if (body->getVertices().empty()) {
        // Fallback: revolve a circle around an offset axis
        const int segs = 32;
        std::vector<glm::dvec3> profile;
        for (int i = 0; i <= segs; ++i) {
            double a = (2.0 * M_PI * i) / segs;
            profile.push_back({majorR + minorR * cos(a), 0, minorR * sin(a)});
        }
        body->revolveProfile(profile, {0, 0, 0}, {0, 0, 1}, 360.0);
    }

    body->uploadToGPU();
    Body3D* ptr = body.get();
    scene.addExistingBody(std::move(body));
    return ptr;
}

// ─────────────────────────────────────────────
// Camera utilities
// ─────────────────────────────────────────────

void Viewport3D::setStandardView(StandardView v) {
    if (!camera) return;
    float dist = glm::length(camera->Position);
    if (dist < 5.0f) dist = 30.0f;

    switch (v) {
        case StandardView::Front:
            camera->Position = glm::vec3(0, 0, dist);
            camera->Yaw = -90.0f; camera->Pitch = 0.0f; break;
        case StandardView::Back:
            camera->Position = glm::vec3(0, 0, -dist);
            camera->Yaw = 90.0f; camera->Pitch = 0.0f; break;
        case StandardView::Left:
            camera->Position = glm::vec3(-dist, 0, 0);
            camera->Yaw = 0.0f; camera->Pitch = 0.0f; break;
        case StandardView::Right:
            camera->Position = glm::vec3(dist, 0, 0);
            camera->Yaw = -180.0f; camera->Pitch = 0.0f; break;
        case StandardView::Top:
            camera->Position = glm::vec3(0, dist, 0.001f);
            camera->Yaw = -90.0f; camera->Pitch = -89.0f; break;
        case StandardView::Bottom:
            camera->Position = glm::vec3(0, -dist, 0.001f);
            camera->Yaw = -90.0f; camera->Pitch = 89.0f; break;
        case StandardView::Isometric:
            camera->Position = glm::vec3(dist*0.577f, dist*0.577f, dist*0.577f);
            camera->Yaw = -135.0f; camera->Pitch = -35.26f; break;
    }
    camera->ProcessMouseMovement(0, 0);
}

void Viewport3D::zoomToFit() {
    if (!camera || scene.getBodyCount() == 0) {
        resetCamera();
        return;
    }
    glm::vec3 sceneMin(1e9f), sceneMax(-1e9f);
    for (const auto& body : scene.getBodies()) {
        sceneMin = glm::min(sceneMin, body->getBoundsMin());
        sceneMax = glm::max(sceneMax, body->getBoundsMax());
    }
    glm::vec3 center = (sceneMin + sceneMax) * 0.5f;
    float radius = glm::length(sceneMax - sceneMin) * 0.5f;
    float dist = radius / std::tan(glm::radians(camera->Zoom * 0.5f));
    camera->Position = center - camera->Front * (dist * 1.5f);
    camera->ProcessMouseMovement(0, 0);
}

// ─────────────────────────────────────────────
// Snap Overlay (drawn over canvas with ImGui ForegroundDrawList)
// ─────────────────────────────────────────────

void Viewport3D::renderSnapOverlay(float canvasX, float canvasY) {
    if (!scene.isSketchActive()) return;
    if (currentSnapType == SnapType::None) return;

    // Project snap point to FBO pixel coords, then offset by canvas position
    const WorkPlane3D& wp = scene.getActiveWorkPlane();
    glm::dvec3 pt3d = wp.to3D(snapPoint2D);
    glm::dvec2 fbo = worldToScreen(pt3d);
    if (fbo.x < 0) return;

    float sx = canvasX + (float)fbo.x;
    float sy = canvasY + (float)fbo.y;

    auto* dl = ImGui::GetForegroundDrawList();
    const float R = 8.0f;

    switch (currentSnapType) {
        case SnapType::Endpoint:
            // Orange hollow square
            dl->AddRect(ImVec2(sx-R, sy-R), ImVec2(sx+R, sy+R),
                        IM_COL32(255, 160, 40, 230), 1.0f, 0, 1.8f);
            dl->AddText(ImVec2(sx+R+3, sy-7), IM_COL32(255,160,40,200), "END");
            break;

        case SnapType::Midpoint:
            // Cyan hollow triangle
            dl->AddTriangle(
                ImVec2(sx, sy - R - 1),
                ImVec2(sx + R + 1, sy + R),
                ImVec2(sx - R - 1, sy + R),
                IM_COL32(80, 220, 255, 230), 1.8f);
            dl->AddText(ImVec2(sx+R+3, sy-7), IM_COL32(80,220,255,200), "MID");
            break;

        case SnapType::Center:
            // Green circle + cross
            dl->AddCircle(ImVec2(sx, sy), R, IM_COL32(60, 230, 110, 230), 16, 1.8f);
            dl->AddLine(ImVec2(sx-R, sy), ImVec2(sx+R, sy), IM_COL32(60,230,110,180), 1.0f);
            dl->AddLine(ImVec2(sx, sy-R), ImVec2(sx, sy+R), IM_COL32(60,230,110,180), 1.0f);
            dl->AddText(ImVec2(sx+R+3, sy-7), IM_COL32(60,230,110,200), "CTR");
            break;

        case SnapType::Origin:
            // White cross
            dl->AddLine(ImVec2(sx-R-2, sy), ImVec2(sx+R+2, sy), IM_COL32(220,220,220,230), 2.0f);
            dl->AddLine(ImVec2(sx, sy-R-2), ImVec2(sx, sy+R+2), IM_COL32(220,220,220,230), 2.0f);
            dl->AddText(ImVec2(sx+R+3, sy-7), IM_COL32(220,220,220,200), "ORG");
            break;

        case SnapType::Grid:
            // Small green filled dot for grid snap
            dl->AddCircleFilled(ImVec2(sx, sy), 3.5f, IM_COL32(100, 200, 80, 200));
            break;

        default:
            break;
    }
}

// ─────────────────────────────────────────────
// ImGui Overlay: View Triad (bottom-left XYZ arrows)
// ─────────────────────────────────────────────

void Viewport3D::renderViewTriad(float canvasX, float canvasY, float canvasW, float canvasH) {
    if (!camera) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // Triad position: bottom-left of canvas with padding
    float cx = canvasX + 50.0f;
    float cy = canvasY + canvasH - 50.0f;
    float axLen = 35.0f;

    // Get camera rotation (view matrix without translation)
    glm::mat4 viewMat = camera->GetViewMatrix();
    glm::mat3 rot(viewMat); // Extract 3x3 rotation

    // Project world axes through camera rotation to screen
    glm::vec3 axes[3] = {{1,0,0}, {0,1,0}, {0,0,1}};
    ImU32 colors[3] = {
        IM_COL32(220, 60, 60, 255),   // X = red
        IM_COL32(60, 200, 60, 255),   // Y = green
        IM_COL32(60, 100, 220, 255)   // Z = blue
    };
    const char* labels[3] = {"X", "Y", "Z"};

    for (int i = 0; i < 3; ++i) {
        glm::vec3 projected = rot * axes[i];
        // Screen space: X right, Y down (ImGui convention)
        float ex = cx + projected.x * axLen;
        float ey = cy - projected.y * axLen;

        dl->AddLine(ImVec2(cx, cy), ImVec2(ex, ey), colors[i], 2.5f);

        // Arrow head
        glm::vec2 dir(projected.x, -projected.y);
        if (glm::length(dir) > 0.01f) {
            dir = glm::normalize(dir);
            glm::vec2 perp(-dir.y, dir.x);
            float hs = 6.0f;
            ImVec2 tip(ex, ey);
            ImVec2 l(ex - dir.x*hs + perp.x*hs*0.4f, ey - dir.y*hs + perp.y*hs*0.4f);
            ImVec2 r(ex - dir.x*hs - perp.x*hs*0.4f, ey - dir.y*hs - perp.y*hs*0.4f);
            dl->AddTriangleFilled(tip, l, r, colors[i]);
        }

        // Label
        float lx = cx + projected.x * (axLen + 14.0f) - 4.0f;
        float ly = cy - projected.y * (axLen + 14.0f) - 6.0f;
        dl->AddText(ImVec2(lx, ly), colors[i], labels[i]);
    }

    // Origin dot
    dl->AddCircleFilled(ImVec2(cx, cy), 3.0f, IM_COL32(200, 200, 200, 180));
}

// ─────────────────────────────────────────────
// ImGui Overlay: ViewCube (top-right clickable cube)
// ─────────────────────────────────────────────

void Viewport3D::renderViewCube(float canvasX, float canvasY, float canvasW, float canvasH) {
    if (!camera) return;

    float cubeSize = 55.0f;
    float cx = canvasX + canvasW - cubeSize - 20.0f;
    float cy = canvasY + cubeSize + 20.0f;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    glm::mat3 rot(camera->GetViewMatrix());

    // Define cube corners in world space
    float s = 0.5f;
    glm::vec3 corners[8] = {
        {-s,-s,-s}, {s,-s,-s}, {s,s,-s}, {-s,s,-s},
        {-s,-s, s}, {s,-s, s}, {s,s, s}, {-s,s, s}
    };

    // Project to screen
    ImVec2 proj[8];
    for (int i = 0; i < 8; ++i) {
        glm::vec3 p = rot * corners[i];
        proj[i] = ImVec2(cx + p.x * cubeSize, cy - p.y * cubeSize);
    }

    // Define the 6 faces with their vertex indices and labels
    struct Face { int v[4]; const char* label; ImU32 color; StandardView view; };
    Face faces[6] = {
        {{4,5,6,7}, "FRONT",  IM_COL32(60,70,90,180),  StandardView::Front},
        {{1,0,3,2}, "BACK",   IM_COL32(50,60,80,180),  StandardView::Back},
        {{0,4,7,3}, "LEFT",   IM_COL32(55,65,85,180),  StandardView::Left},
        {{5,1,2,6}, "RIGHT",  IM_COL32(55,65,85,180),  StandardView::Right},
        {{7,6,2,3}, "TOP",    IM_COL32(65,75,95,180),  StandardView::Top},
        {{0,1,5,4}, "BOTTOM", IM_COL32(45,55,75,180),  StandardView::Bottom},
    };

    // Sort faces by depth (back-to-front rendering)
    float faceDepths[6];
    int order[6] = {0,1,2,3,4,5};
    for (int f = 0; f < 6; ++f) {
        glm::vec3 center(0);
        for (int v = 0; v < 4; ++v) center += corners[faces[f].v[v]];
        center *= 0.25f;
        glm::vec3 p = rot * center;
        faceDepths[f] = p.z;
    }
    // Simple bubble sort
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5-i; ++j)
            if (faceDepths[order[j]] > faceDepths[order[j+1]])
                std::swap(order[j], order[j+1]);

    // Draw faces back-to-front
    ImVec2 mousePos = ImGui::GetMousePos();
    for (int fi = 0; fi < 6; ++fi) {
        int f = order[fi];
        auto& face = faces[f];
        ImVec2 p[4] = { proj[face.v[0]], proj[face.v[1]], proj[face.v[2]], proj[face.v[3]] };

        // Check if mouse is over this face (simple quad test)
        ImVec2 center((p[0].x+p[1].x+p[2].x+p[3].x)*0.25f,
                      (p[0].y+p[1].y+p[2].y+p[3].y)*0.25f);
        float dist = std::sqrt((mousePos.x-center.x)*(mousePos.x-center.x) +
                               (mousePos.y-center.y)*(mousePos.y-center.y));
        bool hovered = (dist < cubeSize * 0.45f) && (faceDepths[f] > 0);

        ImU32 faceColor = hovered ? IM_COL32(80, 120, 180, 220) : face.color;

        dl->AddQuadFilled(p[0], p[1], p[2], p[3], faceColor);
        dl->AddQuad(p[0], p[1], p[2], p[3], IM_COL32(120, 130, 150, 200), 1.0f);

        // Label (only if face is front-facing — depth > 0)
        if (faceDepths[f] > -0.1f) {
            ImVec2 textSize = ImGui::CalcTextSize(face.label);
            dl->AddText(ImVec2(center.x - textSize.x*0.5f, center.y - textSize.y*0.5f),
                       IM_COL32(200, 210, 230, 220), face.label);
        }

        // Click handling
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            setStandardView(face.view);
        }
    }
}

} // namespace Modeling3D
