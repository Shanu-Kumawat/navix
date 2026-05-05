#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace Modeling3D {

/**
 * @brief Defines an oriented plane in 3D space for sketch operations.
 *
 * Used to project 2D sketch geometry onto a 3D work surface.
 * Provides coordinate mapping between 2D sketch space and 3D world space.
 */
class WorkPlane3D {
public:
    WorkPlane3D() = default;
    WorkPlane3D(const glm::dvec3& origin, const glm::dvec3& normal, const glm::dvec3& xDir);

    // Factory methods for standard planes
    static WorkPlane3D XY(const glm::dvec3& origin = glm::dvec3(0));
    static WorkPlane3D XZ(const glm::dvec3& origin = glm::dvec3(0));
    static WorkPlane3D YZ(const glm::dvec3& origin = glm::dvec3(0));

    // Coordinate mapping
    glm::dvec3 to3D(const glm::dvec2& point2D) const;
    glm::dvec2 to2D(const glm::dvec3& point3D) const;

    // Project a 3D point onto this plane
    glm::dvec3 projectPoint(const glm::dvec3& point) const;

    // Ray-plane intersection (returns parameter t, negative if no intersection)
    double intersectRay(const glm::dvec3& rayOrigin, const glm::dvec3& rayDir) const;

    // Accessors
    const glm::dvec3& getOrigin() const { return origin; }
    const glm::dvec3& getNormal() const { return normal; }
    const glm::dvec3& getXDirection() const { return xDir; }
    const glm::dvec3& getYDirection() const { return yDir; }
    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }

    // Get the 4x4 transform matrix (plane-to-world)
    glm::dmat4 getPlaneToWorldMatrix() const;
    glm::dmat4 getWorldToPlaneMatrix() const;

private:
    glm::dvec3 origin{0.0, 0.0, 0.0};
    glm::dvec3 normal{0.0, 0.0, 1.0};   // Plane normal (Z by default = XY plane)
    glm::dvec3 xDir{1.0, 0.0, 0.0};     // Local X direction on the plane
    glm::dvec3 yDir{0.0, 1.0, 0.0};     // Local Y direction on the plane
    std::string name{"Custom"};
};

} // namespace Modeling3D
