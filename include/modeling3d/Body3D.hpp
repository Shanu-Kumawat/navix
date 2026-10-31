#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

// Forward declare OCCT types to keep them out of the header
#ifdef USE_OCCT
class TopoDS_Shape;
#endif

namespace Modeling3D {

class Sketch3D;

/**
 * @brief Boolean operation type applied between two solid bodies.
 */
enum class BoolOp {
    Union,      // A + B
    Subtract,   // A - B
    Intersect   // A ∩ B
};

// Cut-Extrude direction modes
enum class CutDirMode {
    Blind,       // along sketch normal, given depth
    Reversed,    // opposite to sketch normal
    ThroughAll   // both directions, all the way through the target
};

// Hole types (lite wizard)
enum class HoleType {
    Through,
    Blind,
    Counterbore
};

// Pattern kinds
enum class PatternKind {
    Linear,
    Circular
};

/**
 * @brief Vertex with position and normal for OpenGL rendering.
 */
struct Vertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color{0.7f, 0.7f, 0.75f}; // Default metallic gray
};

/**
 * @brief A triangle face index triple.
 */
struct TriangleIndex {
    unsigned int v0, v1, v2;
};

/**
 * @brief Represents a single 3D solid/surface body in the modeling scene.
 *
 * Wraps either an OCCT TopoDS_Shape (when available) or a raw mesh.
 * Stores display mesh (tessellation for OpenGL), material, and selection state.
 */
class Body3D {
public:
    Body3D();
    ~Body3D();

    // Identity
    uint64_t getId() const { return id; }
    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }
    // Kind drives the feature-tree icon: Box/Cylinder/Sphere/Cone/Torus/
    // Extrude/Revolve/Mirror/Pattern/Boolean/Import
    const std::string& getKind() const { return kind; }
    void setKind(const std::string& k) { kind = k; }

    // Display mesh (tessellated for OpenGL rendering)
    const std::vector<Vertex3D>& getVertices() const { return vertices; }
    const std::vector<TriangleIndex>& getTriangles() const { return triangles; }
    const std::vector<glm::vec3>& getEdgeVertices() const { return edgeVertices; }

    // Set mesh data directly (for non-OCCT path or after tessellation)
    void setMesh(std::vector<Vertex3D>&& verts, std::vector<TriangleIndex>&& tris);
    void setEdges(std::vector<glm::vec3>&& edges);

    // Material
    glm::vec3 getColor() const { return color; }
    void setColor(const glm::vec3& c) { color = c; }
    float getOpacity() const { return opacity; }
    void setOpacity(float o) { opacity = o; }

    // Transform
    const glm::mat4& getTransform() const { return transform; }
    void setTransform(const glm::mat4& t) { transform = t; }

    // Selection & visibility
    bool isSelected() const { return selected; }
    void setSelected(bool s) { selected = s; }
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    // Bounding box (AABB in local space)
    void computeBounds();
    glm::vec3 getBoundsMin() const { return boundsMin; }
    glm::vec3 getBoundsMax() const { return boundsMax; }
    glm::vec3 getCenter() const { return (boundsMin + boundsMax) * 0.5f; }

    // OpenGL buffer management
    void uploadToGPU();
    void draw() const;
    void drawEdges() const;
    bool isUploaded() const { return gpuUploaded; }

    // Operations (build geometry)
    // Extrude a closed wire profile along a direction
    bool extrudeProfile(const std::vector<glm::dvec3>& profile,
                        const glm::dvec3& direction, double distance);
    // Revolve a wire profile around an axis
    bool revolveProfile(const std::vector<glm::dvec3>& profile,
                        const glm::dvec3& axisOrigin,
                        const glm::dvec3& axisDir,
                        double angleDegrees);

    // Edge operations (OCCT-only — no-op on mesh fallback)
    bool filletAllEdges(double radius);
    bool filletEdgesByIndex(const std::vector<int>& edgeIndices, double radius);
    bool chamferAllEdges(double distance);

    // Boolean operations (OCCT-only)
    bool booleanUnion(Body3D& other);
    bool booleanSubtract(Body3D& other);
    bool booleanIntersect(Body3D& other);

    // World-space circle profile helper (used by hole/pocket cuts and their
    // ghost previews). Returns `segments` points on the circle centered at
    // `center` with axis `normal`, plus the start point to close the wire.
    static std::vector<glm::dvec3> circleProfileWorld(const glm::dvec3& center,
                                                      const glm::dvec3& normal,
                                                      double radius,
                                                      int segments = 48);

#ifdef USE_OCCT
    // Access underlying OCCT shape
    void setOCCTShape(const TopoDS_Shape& shape);
    const TopoDS_Shape& getOCCTShape() const;
    bool hasOCCTShape() const;

    // Shape with the body's display transform applied (world space).
    // Returns a null shape if no B-Rep shape is present.
    TopoDS_Shape getWorldShape() const;

    // Boolean between this body's world-space shape and a world-space tool
    // shape. The result (in world space) becomes this body's shape and the
    // display transform is reset to identity. Fails if the result is empty
    // (e.g. intersecting disjoint solids).
    bool applyBooleanFromWorldShapes(const TopoDS_Shape& toolWorldShape, BoolOp op);

    // Rebuild the display mesh from the current B-Rep shape
    void tessellateFromOCCT();
#endif

private:
    static uint64_t nextId;
    uint64_t id;
    std::string name{"Body"};
    std::string kind{"Body"};

    // Display mesh
    std::vector<Vertex3D> vertices;
    std::vector<TriangleIndex> triangles;
    std::vector<glm::vec3> edgeVertices; // Line pairs for wireframe edges

    // Material
    glm::vec3 color{0.6f, 0.65f, 0.72f};
    float opacity{1.0f};

    // Transform
    glm::mat4 transform{1.0f};

    // State
    bool selected{false};
    bool visible{true};

    // Bounds
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};

    // OpenGL
    unsigned int VAO{0}, VBO{0}, EBO{0};
    unsigned int edgeVAO{0}, edgeVBO{0};
    bool gpuUploaded{false};

#ifdef USE_OCCT
    struct OCCTData;
    std::unique_ptr<OCCTData> occtData;
#endif

    // Helper: build mesh from profile extrusion (non-OCCT fallback)
    void buildExtrudeMesh(const std::vector<glm::dvec3>& profile,
                          const glm::dvec3& direction, double distance);
    // Helper: build mesh from profile revolution (non-OCCT fallback)
    void buildRevolveMesh(const std::vector<glm::dvec3>& profile,
                          const glm::dvec3& axisOrigin,
                          const glm::dvec3& axisDir,
                          double angleDegrees);
};

} // namespace Modeling3D
