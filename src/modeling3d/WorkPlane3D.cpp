#include "modeling3d/WorkPlane3D.hpp"
#include <glm/gtc/matrix_inverse.hpp>

namespace Modeling3D {

WorkPlane3D::WorkPlane3D(const glm::dvec3& origin, const glm::dvec3& normal, const glm::dvec3& xDir)
    : origin(origin), normal(glm::normalize(normal)), xDir(glm::normalize(xDir))
{
    // Ensure xDir is perpendicular to normal
    this->xDir = glm::normalize(this->xDir - glm::dot(this->xDir, this->normal) * this->normal);
    this->yDir = glm::cross(this->normal, this->xDir);
}

WorkPlane3D WorkPlane3D::XY(const glm::dvec3& origin) {
    WorkPlane3D wp(origin, glm::dvec3(0, 0, 1), glm::dvec3(1, 0, 0));
    wp.setName("XY Plane");
    return wp;
}

WorkPlane3D WorkPlane3D::XZ(const glm::dvec3& origin) {
    WorkPlane3D wp(origin, glm::dvec3(0, 1, 0), glm::dvec3(1, 0, 0));
    wp.setName("XZ Plane");
    return wp;
}

WorkPlane3D WorkPlane3D::YZ(const glm::dvec3& origin) {
    WorkPlane3D wp(origin, glm::dvec3(1, 0, 0), glm::dvec3(0, 1, 0));
    wp.setName("YZ Plane");
    return wp;
}

glm::dvec3 WorkPlane3D::to3D(const glm::dvec2& point2D) const {
    return origin + point2D.x * xDir + point2D.y * yDir;
}

glm::dvec2 WorkPlane3D::to2D(const glm::dvec3& point3D) const {
    glm::dvec3 local = point3D - origin;
    return glm::dvec2(glm::dot(local, xDir), glm::dot(local, yDir));
}

glm::dvec3 WorkPlane3D::projectPoint(const glm::dvec3& point) const {
    double dist = glm::dot(point - origin, normal);
    return point - dist * normal;
}

double WorkPlane3D::intersectRay(const glm::dvec3& rayOrigin, const glm::dvec3& rayDir) const {
    double denom = glm::dot(normal, rayDir);
    if (std::abs(denom) < 1e-10) return -1.0; // Ray parallel to plane
    return glm::dot(origin - rayOrigin, normal) / denom;
}

glm::dmat4 WorkPlane3D::getPlaneToWorldMatrix() const {
    glm::dmat4 m(1.0);
    m[0] = glm::dvec4(xDir, 0.0);
    m[1] = glm::dvec4(yDir, 0.0);
    m[2] = glm::dvec4(normal, 0.0);
    m[3] = glm::dvec4(origin, 1.0);
    return m;
}

glm::dmat4 WorkPlane3D::getWorldToPlaneMatrix() const {
    return glm::affineInverse(getPlaneToWorldMatrix());
}

} // namespace Modeling3D
