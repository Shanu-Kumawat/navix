#include "modeling3d/Body3D.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

#ifdef USE_OCCT
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <TopLoc_Location.hxx>
#include <Geom_Curve.hxx>
#include <Standard_Failure.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

namespace Modeling3D {
struct Body3D::OCCTData {
    TopoDS_Shape shape;
    bool hasShape{false};
};
}
#endif

namespace Modeling3D {

uint64_t Body3D::nextId = 1;

Body3D::Body3D() : id(nextId++) {
    name = "Body " + std::to_string(id);
#ifdef USE_OCCT
    occtData = std::make_unique<OCCTData>();
#endif
}

Body3D::~Body3D() {
    if (gpuUploaded) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        if (edgeVAO) {
            glDeleteVertexArrays(1, &edgeVAO);
            glDeleteBuffers(1, &edgeVBO);
        }
    }
}

void Body3D::setMesh(std::vector<Vertex3D>&& verts, std::vector<TriangleIndex>&& tris) {
    vertices = std::move(verts);
    triangles = std::move(tris);
    gpuUploaded = false;
    computeBounds();
}

void Body3D::setEdges(std::vector<glm::vec3>&& edges) {
    edgeVertices = std::move(edges);
}

void Body3D::computeBounds() {
    if (vertices.empty()) {
        boundsMin = boundsMax = glm::vec3(0);
        return;
    }
    boundsMin = boundsMax = vertices[0].position;
    for (const auto& v : vertices) {
        boundsMin = glm::min(boundsMin, v.position);
        boundsMax = glm::max(boundsMax, v.position);
    }
}

void Body3D::uploadToGPU() {
    if (vertices.empty() || triangles.empty()) return;

    if (gpuUploaded) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(TriangleIndex),
                 triangles.data(), GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, position));
    glEnableVertexAttribArray(0);

    // Normal attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, normal));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Upload edge vertices if present
    if (!edgeVertices.empty()) {
        if (edgeVAO) {
            glDeleteVertexArrays(1, &edgeVAO);
            glDeleteBuffers(1, &edgeVBO);
        }
        glGenVertexArrays(1, &edgeVAO);
        glGenBuffers(1, &edgeVBO);
        glBindVertexArray(edgeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, edgeVBO);
        glBufferData(GL_ARRAY_BUFFER, edgeVertices.size() * sizeof(glm::vec3),
                     edgeVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    gpuUploaded = true;
}

void Body3D::draw() const {
    if (!gpuUploaded || triangles.empty()) return;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangles.size() * 3),
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Body3D::drawEdges() const {
    if (!gpuUploaded || edgeVertices.empty()) return;
    glBindVertexArray(edgeVAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(edgeVertices.size()));
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────
// Extrude / Revolve operations
// ─────────────────────────────────────────────

bool Body3D::extrudeProfile(const std::vector<glm::dvec3>& profile,
                             const glm::dvec3& direction, double distance) {
    if (profile.size() < 3) return false;

#ifdef USE_OCCT
    try {
        // Build wire from profile points
        BRepBuilderAPI_MakeWire wireBuilder;
        for (size_t i = 0; i < profile.size() - 1; ++i) {
            gp_Pnt p1(profile[i].x, profile[i].y, profile[i].z);
            gp_Pnt p2(profile[i+1].x, profile[i+1].y, profile[i+1].z);
            if (p1.Distance(p2) < 1e-8) continue;
            wireBuilder.Add(BRepBuilderAPI_MakeEdge(p1, p2));
        }
        if (!wireBuilder.IsDone()) return false;

        // Make face from wire
        BRepBuilderAPI_MakeFace faceBuilder(wireBuilder.Wire());
        if (!faceBuilder.IsDone()) return false;

        // Extrude
        glm::dvec3 extVec = glm::normalize(direction) * distance;
        gp_Vec vec(extVec.x, extVec.y, extVec.z);
        BRepPrimAPI_MakePrism prism(faceBuilder.Face(), vec);
        if (!prism.IsDone()) return false;

        setOCCTShape(prism.Shape());
        tessellateFromOCCT();
        return true;
    } catch (...) {
        std::cerr << "OCCT extrude failed, falling back to mesh" << std::endl;
    }
#endif

    // Fallback: software mesh extrusion
    buildExtrudeMesh(profile, direction, distance);
    return true;
}

bool Body3D::revolveProfile(const std::vector<glm::dvec3>& profile,
                             const glm::dvec3& axisOrigin,
                             const glm::dvec3& axisDir,
                             double angleDegrees) {
    if (profile.size() < 2) return false;

#ifdef USE_OCCT
    try {
        BRepBuilderAPI_MakeWire wireBuilder;
        for (size_t i = 0; i < profile.size() - 1; ++i) {
            gp_Pnt p1(profile[i].x, profile[i].y, profile[i].z);
            gp_Pnt p2(profile[i+1].x, profile[i+1].y, profile[i+1].z);
            if (p1.Distance(p2) < 1e-8) continue;
            wireBuilder.Add(BRepBuilderAPI_MakeEdge(p1, p2));
        }
        if (!wireBuilder.IsDone()) return false;

        BRepBuilderAPI_MakeFace faceBuilder(wireBuilder.Wire());
        if (!faceBuilder.IsDone()) return false;

        gp_Ax1 axis(gp_Pnt(axisOrigin.x, axisOrigin.y, axisOrigin.z),
                     gp_Dir(axisDir.x, axisDir.y, axisDir.z));
        double angleRad = angleDegrees * M_PI / 180.0;
        BRepPrimAPI_MakeRevol revol(faceBuilder.Face(), axis, angleRad);
        if (!revol.IsDone()) return false;

        setOCCTShape(revol.Shape());
        tessellateFromOCCT();
        return true;
    } catch (...) {
        std::cerr << "OCCT revolve failed, falling back to mesh" << std::endl;
    }
#endif

    buildRevolveMesh(profile, axisOrigin, axisDir, angleDegrees);
    return true;
}

#ifdef USE_OCCT
void Body3D::setOCCTShape(const TopoDS_Shape& shape) {
    occtData->shape = shape;
    occtData->hasShape = true;
}

const TopoDS_Shape& Body3D::getOCCTShape() const {
    return occtData->shape;
}

bool Body3D::hasOCCTShape() const {
    return occtData && occtData->hasShape;
}

void Body3D::tessellateFromOCCT() {
    if (!occtData || !occtData->hasShape) return;

    vertices.clear();
    triangles.clear();
    edgeVertices.clear();

    // Mesh the shape
    BRepMesh_IncrementalMesh mesh(occtData->shape, 0.5); // Linear deflection
    mesh.Perform();

    // Extract triangulation from faces
    unsigned int vertexOffset = 0;
    for (TopExp_Explorer exp(occtData->shape, TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Face& face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;

        unsigned int baseIdx = static_cast<unsigned int>(vertices.size());

        // Add vertices (apply location transform)
        gp_Trsf trsf = loc.Transformation();
        for (int i = 1; i <= tri->NbNodes(); ++i) {
            gp_Pnt p = tri->Node(i);
            p.Transform(trsf);
            Vertex3D v;
            v.position = glm::vec3(static_cast<float>(p.X()),
                                   static_cast<float>(p.Y()),
                                   static_cast<float>(p.Z()));
            v.normal = glm::vec3(0, 1, 0); // Will be computed below
            v.color = color;
            vertices.push_back(v);
        }

        // Add triangles
        for (int i = 1; i <= tri->NbTriangles(); ++i) {
            int n1, n2, n3;
            tri->Triangle(i).Get(n1, n2, n3);
            TriangleIndex ti;
            ti.v0 = baseIdx + n1 - 1;
            ti.v1 = baseIdx + n2 - 1;
            ti.v2 = baseIdx + n3 - 1;
            triangles.push_back(ti);
        }
    }

    // Compute normals from faces
    for (const auto& tri : triangles) {
        if (tri.v0 >= vertices.size() || tri.v1 >= vertices.size() || tri.v2 >= vertices.size())
            continue;
        glm::vec3 e1 = vertices[tri.v1].position - vertices[tri.v0].position;
        glm::vec3 e2 = vertices[tri.v2].position - vertices[tri.v0].position;
        glm::vec3 n = glm::normalize(glm::cross(e1, e2));
        vertices[tri.v0].normal += n;
        vertices[tri.v1].normal += n;
        vertices[tri.v2].normal += n;
    }
    for (auto& v : vertices) {
        if (glm::length(v.normal) > 0.001f)
            v.normal = glm::normalize(v.normal);
    }

    // Extract edges for wireframe
    for (TopExp_Explorer exp(occtData->shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(exp.Current());
        TopLoc_Location loc;
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, loc, first, last);
        if (curve.IsNull()) continue;

        gp_Trsf edgeTrsf = loc.Transformation();
        const int nSteps = 20;
        for (int i = 0; i < nSteps; ++i) {
            double t1 = first + (last - first) * i / nSteps;
            double t2 = first + (last - first) * (i + 1) / nSteps;
            gp_Pnt p1 = curve->Value(t1);
            gp_Pnt p2 = curve->Value(t2);
            p1.Transform(edgeTrsf);
            p2.Transform(edgeTrsf);
            edgeVertices.push_back(glm::vec3(static_cast<float>(p1.X()),
                                             static_cast<float>(p1.Y()),
                                             static_cast<float>(p1.Z())));
            edgeVertices.push_back(glm::vec3(static_cast<float>(p2.X()),
                                             static_cast<float>(p2.Y()),
                                             static_cast<float>(p2.Z())));
        }
    }

    computeBounds();
    gpuUploaded = false;
}
#endif

// ─────────────────────────────────────────────
// Fillet / Chamfer (OCCT required)
// ─────────────────────────────────────────────

bool Body3D::filletAllEdges(double radius) {
#ifdef USE_OCCT
    if (!occtData || !occtData->hasShape) return false;
    if (radius <= 0.0) return false;

    try {

        BRepFilletAPI_MakeFillet fillet(occtData->shape);

        // Add all edges with the specified radius
        for (TopExp_Explorer exp(occtData->shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            fillet.Add(radius, TopoDS::Edge(exp.Current()));
        }

        fillet.Build();
        if (!fillet.IsDone()) {
            std::cerr << "Fillet operation failed (radius may be too large)" << std::endl;
            return false;
        }

        setOCCTShape(fillet.Shape());
        tessellateFromOCCT();
        gpuUploaded = false;
        return true;
    } catch (const Standard_Failure& e) {
        std::cerr << "OCCT Fillet error: " << e.GetMessageString() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "OCCT Fillet error (unknown)" << std::endl;
        return false;
    }
#else
    (void)radius;
    std::cerr << "Fillet requires OpenCASCADE" << std::endl;
    return false;
#endif
}

bool Body3D::chamferAllEdges(double distance) {
#ifdef USE_OCCT
    if (!occtData || !occtData->hasShape) return false;
    if (distance <= 0.0) return false;

    try {

        BRepFilletAPI_MakeChamfer chamfer(occtData->shape);

        for (TopExp_Explorer exp(occtData->shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            chamfer.Add(distance, TopoDS::Edge(exp.Current()));
        }

        chamfer.Build();
        if (!chamfer.IsDone()) {
            std::cerr << "Chamfer operation failed" << std::endl;
            return false;
        }

        setOCCTShape(chamfer.Shape());
        tessellateFromOCCT();
        gpuUploaded = false;
        return true;
    } catch (const Standard_Failure& e) {
        std::cerr << "OCCT Chamfer error: " << e.GetMessageString() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "OCCT Chamfer error (unknown)" << std::endl;
        return false;
    }
#else
    (void)distance;
    std::cerr << "Chamfer requires OpenCASCADE" << std::endl;
    return false;
#endif
}

// ─────────────────────────────────────────────
// Boolean Operations (OCCT required)
// ─────────────────────────────────────────────

bool Body3D::booleanUnion(Body3D& other) {
#ifdef USE_OCCT
    if (!occtData || !occtData->hasShape || !other.hasOCCTShape()) return false;

    try {

        BRepAlgoAPI_Fuse fuse(occtData->shape, other.getOCCTShape());
        if (!fuse.IsDone()) return false;

        setOCCTShape(fuse.Shape());
        tessellateFromOCCT();
        gpuUploaded = false;
        return true;
    } catch (...) {
        std::cerr << "Boolean Union failed" << std::endl;
        return false;
    }
#else
    (void)other;
    return false;
#endif
}

bool Body3D::booleanSubtract(Body3D& other) {
#ifdef USE_OCCT
    if (!occtData || !occtData->hasShape || !other.hasOCCTShape()) return false;

    try {

        BRepAlgoAPI_Cut cut(occtData->shape, other.getOCCTShape());
        if (!cut.IsDone()) return false;

        setOCCTShape(cut.Shape());
        tessellateFromOCCT();
        gpuUploaded = false;
        return true;
    } catch (...) {
        std::cerr << "Boolean Subtract failed" << std::endl;
        return false;
    }
#else
    (void)other;
    return false;
#endif
}

bool Body3D::booleanIntersect(Body3D& other) {
#ifdef USE_OCCT
    if (!occtData || !occtData->hasShape || !other.hasOCCTShape()) return false;

    try {

        BRepAlgoAPI_Common common(occtData->shape, other.getOCCTShape());
        if (!common.IsDone()) return false;

        setOCCTShape(common.Shape());
        tessellateFromOCCT();
        gpuUploaded = false;
        return true;
    } catch (...) {
        std::cerr << "Boolean Intersect failed" << std::endl;
        return false;
    }
#else
    (void)other;
    return false;
#endif
}

// ─────────────────────────────────────────────
// Fallback mesh builders (no OCCT)
// ─────────────────────────────────────────────

void Body3D::buildExtrudeMesh(const std::vector<glm::dvec3>& profile,
                               const glm::dvec3& direction, double distance) {
    vertices.clear();
    triangles.clear();
    edgeVertices.clear();

    glm::dvec3 extVec = glm::normalize(direction) * distance;
    int n = static_cast<int>(profile.size());

    // Front face vertices (z = 0)
    for (int i = 0; i < n; ++i) {
        Vertex3D v;
        v.position = glm::vec3(profile[i]);
        v.normal = glm::vec3(-glm::normalize(extVec));
        v.color = color;
        vertices.push_back(v);
    }

    // Back face vertices (z = distance)
    for (int i = 0; i < n; ++i) {
        Vertex3D v;
        v.position = glm::vec3(glm::dvec3(profile[i]) + extVec);
        v.normal = glm::vec3(glm::normalize(extVec));
        v.color = color;
        vertices.push_back(v);
    }

    // Triangle-fan for front face (simple convex assumption)
    for (int i = 1; i < n - 1; ++i) {
        triangles.push_back({0, (unsigned)i + 1, (unsigned)i});
    }

    // Triangle-fan for back face
    unsigned int backBase = n;
    for (int i = 1; i < n - 1; ++i) {
        triangles.push_back({backBase, backBase + (unsigned)i, backBase + (unsigned)i + 1});
    }

    // Side faces (quads as 2 triangles)
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;

        // Compute side normal
        glm::dvec3 edge = profile[next] - profile[i];
        glm::dvec3 sideNormal = glm::normalize(glm::cross(edge, extVec));

        unsigned int base = static_cast<unsigned int>(vertices.size());

        Vertex3D v0, v1, v2, v3;
        v0.position = glm::vec3(profile[i]); v0.normal = glm::vec3(sideNormal); v0.color = color;
        v1.position = glm::vec3(profile[next]); v1.normal = glm::vec3(sideNormal); v1.color = color;
        v2.position = glm::vec3(glm::dvec3(profile[next]) + extVec); v2.normal = glm::vec3(sideNormal); v2.color = color;
        v3.position = glm::vec3(glm::dvec3(profile[i]) + extVec); v3.normal = glm::vec3(sideNormal); v3.color = color;

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);

        triangles.push_back({base, base + 1, base + 2});
        triangles.push_back({base, base + 2, base + 3});
    }

    // Edge lines for wireframe
    for (int i = 0; i < n; ++i) {
        int next = (i + 1) % n;
        // Front edges
        edgeVertices.push_back(glm::vec3(profile[i]));
        edgeVertices.push_back(glm::vec3(profile[next]));
        // Back edges
        edgeVertices.push_back(glm::vec3(glm::dvec3(profile[i]) + extVec));
        edgeVertices.push_back(glm::vec3(glm::dvec3(profile[next]) + extVec));
        // Side edges
        edgeVertices.push_back(glm::vec3(profile[i]));
        edgeVertices.push_back(glm::vec3(glm::dvec3(profile[i]) + extVec));
    }

    computeBounds();
    gpuUploaded = false;
}

void Body3D::buildRevolveMesh(const std::vector<glm::dvec3>& profile,
                               const glm::dvec3& axisOrigin,
                               const glm::dvec3& axisDir,
                               double angleDegrees) {
    vertices.clear();
    triangles.clear();
    edgeVertices.clear();

    glm::dvec3 axis = glm::normalize(axisDir);
    double angleRad = glm::radians(angleDegrees);
    int segments = std::max(16, (int)(angleDegrees / 5.0));
    int n = static_cast<int>(profile.size());

    // Generate rotation rings
    for (int s = 0; s <= segments; ++s) {
        double t = (double)s / segments;
        double angle = t * angleRad;
        glm::dmat4 rot = glm::rotate(glm::dmat4(1.0), angle, axis);

        for (int i = 0; i < n; ++i) {
            glm::dvec3 local = profile[i] - axisOrigin;
            glm::dvec3 rotated = glm::dvec3(rot * glm::dvec4(local, 1.0)) + axisOrigin;

            Vertex3D v;
            v.position = glm::vec3(rotated);
            v.normal = glm::vec3(0, 1, 0); // Will be recomputed
            v.color = color;
            vertices.push_back(v);
        }
    }

    // Generate triangles between rings
    for (int s = 0; s < segments; ++s) {
        for (int i = 0; i < n - 1; ++i) {
            unsigned int curr = s * n + i;
            unsigned int next = s * n + i + 1;
            unsigned int currNext = (s + 1) * n + i;
            unsigned int nextNext = (s + 1) * n + i + 1;

            triangles.push_back({curr, next, nextNext});
            triangles.push_back({curr, nextNext, currNext});
        }
    }

    // Recompute normals
    for (auto& v : vertices) v.normal = glm::vec3(0);
    for (const auto& tri : triangles) {
        glm::vec3 e1 = vertices[tri.v1].position - vertices[tri.v0].position;
        glm::vec3 e2 = vertices[tri.v2].position - vertices[tri.v0].position;
        glm::vec3 fn = glm::normalize(glm::cross(e1, e2));
        vertices[tri.v0].normal += fn;
        vertices[tri.v1].normal += fn;
        vertices[tri.v2].normal += fn;
    }
    for (auto& v : vertices) {
        if (glm::length(v.normal) > 0.001f)
            v.normal = glm::normalize(v.normal);
    }

    // Edges: profile outlines at each ring
    for (int s = 0; s <= segments; s += std::max(1, segments / 8)) {
        for (int i = 0; i < n - 1; ++i) {
            unsigned int idx = s * n + i;
            edgeVertices.push_back(vertices[idx].position);
            edgeVertices.push_back(vertices[idx + 1].position);
        }
    }
    // Edges: circumferential at each profile point
    for (int i = 0; i < n; i += std::max(1, n / 4)) {
        for (int s = 0; s < segments; ++s) {
            unsigned int curr = s * n + i;
            unsigned int next = (s + 1) * n + i;
            edgeVertices.push_back(vertices[curr].position);
            edgeVertices.push_back(vertices[next].position);
        }
    }

    computeBounds();
    gpuUploaded = false;
}

} // namespace Modeling3D
