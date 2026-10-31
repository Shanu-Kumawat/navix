#include "modeling3d/Exporter3D.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <functional>
#include <glm/gtc/type_ptr.hpp>

#ifdef USE_OCCT
#include <STEPControl_Writer.hxx>
#include <IGESControl_Writer.hxx>
#include <STEPControl_Reader.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopoDS_Shape.hxx>
#include <Interface_Static.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#endif

namespace Modeling3D {

// ─────────────────────────────────────────────
// STL Export
// ─────────────────────────────────────────────

bool Exporter3D::exportSTL(const Body3D& body, const std::string& filepath, bool binary) {
    std::ofstream file;
    if (binary) {
        file.open(filepath, std::ios::binary);
    } else {
        file.open(filepath);
    }
    if (!file.is_open()) {
        std::cerr << "Exporter3D: Cannot open file: " << filepath << std::endl;
        return false;
    }

    if (binary) {
        // Header (80 bytes)
        char header[80] = {};
        std::string headerStr = "NAVIX STL Export - " + body.getName();
        std::strncpy(header, headerStr.c_str(), 79);
        file.write(header, 80);

        // Triangle count placeholder
        uint32_t triCount = 0;
        auto countPos = file.tellp();
        file.write(reinterpret_cast<const char*>(&triCount), 4);

        writeSTLBinary(body, file, triCount);

        // Write actual triangle count
        file.seekp(countPos);
        file.write(reinterpret_cast<const char*>(&triCount), 4);
    } else {
        file << "solid " << body.getName() << "\n";
        writeSTLAscii(body, file);
        file << "endsolid " << body.getName() << "\n";
    }

    file.close();
    std::cout << "Exported STL: " << filepath << std::endl;
    return true;
}

bool Exporter3D::exportSTL(const std::vector<Body3D*>& bodies,
                            const std::string& filepath, bool binary) {
    if (bodies.empty()) return false;

    std::ofstream file;
    if (binary) {
        file.open(filepath, std::ios::binary);
    } else {
        file.open(filepath);
    }
    if (!file.is_open()) return false;

    if (binary) {
        char header[80] = {};
        std::strncpy(header, "NAVIX Multi-Body STL Export", 79);
        file.write(header, 80);

        uint32_t totalTris = 0;
        auto countPos = file.tellp();
        file.write(reinterpret_cast<const char*>(&totalTris), 4);

        for (const auto* body : bodies) {
            if (body) writeSTLBinary(*body, file, totalTris);
        }

        file.seekp(countPos);
        file.write(reinterpret_cast<const char*>(&totalTris), 4);
    } else {
        file << "solid navix_scene\n";
        for (const auto* body : bodies) {
            if (body) writeSTLAscii(*body, file);
        }
        file << "endsolid navix_scene\n";
    }

    file.close();
    return true;
}

void Exporter3D::writeSTLAscii(const Body3D& body, std::ostream& out) {
    const auto& verts = body.getVertices();
    const auto& tris = body.getTriangles();
    glm::mat4 xform = body.getTransform();

    for (const auto& tri : tris) {
        if (tri.v0 >= verts.size() || tri.v1 >= verts.size() || tri.v2 >= verts.size())
            continue;

        glm::vec3 p0 = glm::vec3(xform * glm::vec4(verts[tri.v0].position, 1.0f));
        glm::vec3 p1 = glm::vec3(xform * glm::vec4(verts[tri.v1].position, 1.0f));
        glm::vec3 p2 = glm::vec3(xform * glm::vec4(verts[tri.v2].position, 1.0f));

        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        if (std::isnan(normal.x)) normal = glm::vec3(0, 0, 1);

        out << "  facet normal " << normal.x << " " << normal.y << " " << normal.z << "\n";
        out << "    outer loop\n";
        out << "      vertex " << p0.x << " " << p0.y << " " << p0.z << "\n";
        out << "      vertex " << p1.x << " " << p1.y << " " << p1.z << "\n";
        out << "      vertex " << p2.x << " " << p2.y << " " << p2.z << "\n";
        out << "    endloop\n";
        out << "  endfacet\n";
    }
}

void Exporter3D::writeSTLBinary(const Body3D& body, std::ostream& out,
                                 uint32_t& triangleCount) {
    const auto& verts = body.getVertices();
    const auto& tris = body.getTriangles();
    glm::mat4 xform = body.getTransform();

    for (const auto& tri : tris) {
        if (tri.v0 >= verts.size() || tri.v1 >= verts.size() || tri.v2 >= verts.size())
            continue;

        glm::vec3 p0 = glm::vec3(xform * glm::vec4(verts[tri.v0].position, 1.0f));
        glm::vec3 p1 = glm::vec3(xform * glm::vec4(verts[tri.v1].position, 1.0f));
        glm::vec3 p2 = glm::vec3(xform * glm::vec4(verts[tri.v2].position, 1.0f));

        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        if (std::isnan(normal.x)) normal = glm::vec3(0, 0, 1);

        // Normal (3 floats)
        out.write(reinterpret_cast<const char*>(&normal.x), 4);
        out.write(reinterpret_cast<const char*>(&normal.y), 4);
        out.write(reinterpret_cast<const char*>(&normal.z), 4);
        // Vertex 1
        out.write(reinterpret_cast<const char*>(&p0.x), 4);
        out.write(reinterpret_cast<const char*>(&p0.y), 4);
        out.write(reinterpret_cast<const char*>(&p0.z), 4);
        // Vertex 2
        out.write(reinterpret_cast<const char*>(&p1.x), 4);
        out.write(reinterpret_cast<const char*>(&p1.y), 4);
        out.write(reinterpret_cast<const char*>(&p1.z), 4);
        // Vertex 3
        out.write(reinterpret_cast<const char*>(&p2.x), 4);
        out.write(reinterpret_cast<const char*>(&p2.y), 4);
        out.write(reinterpret_cast<const char*>(&p2.z), 4);
        // Attribute byte count
        uint16_t attr = 0;
        out.write(reinterpret_cast<const char*>(&attr), 2);

        ++triangleCount;
    }
}

// ─────────────────────────────────────────────
// OBJ Export
// ─────────────────────────────────────────────

// Raw indexed-mesh export (Base3DModel display meshes)
bool Exporter3D::exportSTL(const std::vector<float>& vertexData,
                           const std::vector<unsigned int>& indices,
                           const std::string& filepath,
                           const std::string& name,
                           bool binary) {
    if (vertexData.size() < 6 || indices.size() < 3) {
        std::cerr << "Exporter3D: empty mesh, nothing to export" << std::endl;
        return false;
    }
    auto vertexCount = vertexData.size() / 6;

    std::ofstream file;
    file.open(filepath, binary ? std::ios::binary : std::ios::out);
    if (!file.is_open()) {
        std::cerr << "Exporter3D: Cannot open file: " << filepath << std::endl;
        return false;
    }

    auto emitTriangle = [&](unsigned int a, unsigned int b, unsigned int c,
                            const std::function<void(const glm::vec3&, const glm::vec3&,
                                                     const glm::vec3&, const glm::vec3&)>& emit) {
        auto P = [&](unsigned int i) -> glm::vec3 {
            return glm::vec3(vertexData[i * 6 + 0], vertexData[i * 6 + 1], vertexData[i * 6 + 2]);
        };
        glm::vec3 p0 = P(a), p1 = P(b), p2 = P(c);
        glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        if (std::isnan(n.x)) n = glm::vec3(0, 0, 1);
        emit(n, p0, p1, p2);
    };

    if (binary) {
        char header[80] = {};
        std::strncpy(header, ("NAVIX STL Export - " + name).c_str(), 79);
        file.write(header, 80);
        uint32_t triCount = 0;
        auto countPos = file.tellp();
        file.write(reinterpret_cast<const char*>(&triCount), 4);

        for (size_t t = 0; t + 2 < indices.size(); t += 3) {
            if (indices[t] >= vertexCount || indices[t+1] >= vertexCount || indices[t+2] >= vertexCount)
                continue;
            emitTriangle(indices[t], indices[t+1], indices[t+2],
                [&](const glm::vec3& n, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
                    file.write(reinterpret_cast<const char*>(&n.x), 4);
                    file.write(reinterpret_cast<const char*>(&n.y), 4);
                    file.write(reinterpret_cast<const char*>(&n.z), 4);
                    file.write(reinterpret_cast<const char*>(&p0.x), 4);
                    file.write(reinterpret_cast<const char*>(&p0.y), 4);
                    file.write(reinterpret_cast<const char*>(&p0.z), 4);
                    file.write(reinterpret_cast<const char*>(&p1.x), 4);
                    file.write(reinterpret_cast<const char*>(&p1.y), 4);
                    file.write(reinterpret_cast<const char*>(&p1.z), 4);
                    file.write(reinterpret_cast<const char*>(&p2.x), 4);
                    file.write(reinterpret_cast<const char*>(&p2.y), 4);
                    file.write(reinterpret_cast<const char*>(&p2.z), 4);
                    uint16_t attr = 0;
                    file.write(reinterpret_cast<const char*>(&attr), 2);
                    ++triCount;
                });
        }
        file.seekp(countPos);
        file.write(reinterpret_cast<const char*>(&triCount), 4);
    } else {
        file << "solid " << name << "\n";
        for (size_t t = 0; t + 2 < indices.size(); t += 3) {
            if (indices[t] >= vertexCount || indices[t+1] >= vertexCount || indices[t+2] >= vertexCount)
                continue;
            emitTriangle(indices[t], indices[t+1], indices[t+2],
                [&](const glm::vec3& n, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
                    file << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n";
                    file << "    outer loop\n";
                    file << "      vertex " << p0.x << " " << p0.y << " " << p0.z << "\n";
                    file << "      vertex " << p1.x << " " << p1.y << " " << p1.z << "\n";
                    file << "      vertex " << p2.x << " " << p2.y << " " << p2.z << "\n";
                    file << "    endloop\n";
                    file << "  endfacet\n";
                });
        }
        file << "endsolid " << name << "\n";
    }
    file.close();
    std::cout << "Exported STL: " << filepath << std::endl;
    return true;
}

bool Exporter3D::exportOBJ(const std::vector<float>& vertexData,
                           const std::vector<unsigned int>& indices,
                           const std::string& filepath,
                           const std::string& name) {
    if (vertexData.size() < 6 || indices.size() < 3) {
        std::cerr << "Exporter3D: empty mesh, nothing to export" << std::endl;
        return false;
    }
    std::ofstream out(filepath);
    if (!out.is_open()) {
        std::cerr << "Exporter3D: Cannot open file: " << filepath << std::endl;
        return false;
    }
    out << "o " << name << "\n";
    for (size_t i = 0; i + 5 < vertexData.size(); i += 6) {
        out << "v " << vertexData[i] << " " << vertexData[i+1] << " " << vertexData[i+2] << "\n";
    }
    for (size_t i = 0; i + 5 < vertexData.size(); i += 6) {
        out << "vn " << vertexData[i+3] << " " << vertexData[i+4] << " " << vertexData[i+5] << "\n";
    }
    size_t vertexCount = vertexData.size() / 6;
    for (size_t t = 0; t + 2 < indices.size(); t += 3) {
        if (indices[t] >= vertexCount || indices[t+1] >= vertexCount || indices[t+2] >= vertexCount)
            continue;
        // OBJ is 1-based
        out << "f " << (indices[t] + 1) << "//" << (indices[t] + 1) << " "
                   << (indices[t+1] + 1) << "//" << (indices[t+1] + 1) << " "
                   << (indices[t+2] + 1) << "//" << (indices[t+2] + 1) << "\n";
    }
    out.close();
    std::cout << "Exported OBJ: " << filepath << std::endl;
    return true;
}

bool Exporter3D::exportOBJ(const std::vector<Body3D*>& bodies,
                            const std::string& filepath) {
    if (bodies.empty()) return false;

    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# NAVIX OBJ Export\n";
    file << "# Bodies: " << bodies.size() << "\n\n";

    unsigned int vertexOffset = 0;

    for (const auto* body : bodies) {
        if (!body) continue;

        const auto& verts = body->getVertices();
        const auto& tris = body->getTriangles();
        glm::mat4 xform = body->getTransform();

        file << "o " << body->getName() << "\n";

        // Vertices
        for (const auto& v : verts) {
            glm::vec3 p = glm::vec3(xform * glm::vec4(v.position, 1.0f));
            file << "v " << p.x << " " << p.y << " " << p.z << "\n";
        }

        // Normals
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(xform)));
        for (const auto& v : verts) {
            glm::vec3 n = glm::normalize(normalMatrix * v.normal);
            file << "vn " << n.x << " " << n.y << " " << n.z << "\n";
        }

        // Faces (1-indexed)
        for (const auto& tri : tris) {
            unsigned int i0 = vertexOffset + tri.v0 + 1;
            unsigned int i1 = vertexOffset + tri.v1 + 1;
            unsigned int i2 = vertexOffset + tri.v2 + 1;
            file << "f " << i0 << "//" << i0 << " "
                         << i1 << "//" << i1 << " "
                         << i2 << "//" << i2 << "\n";
        }

        vertexOffset += static_cast<unsigned int>(verts.size());
        file << "\n";
    }

    file.close();
    std::cout << "Exported OBJ: " << filepath << std::endl;
    return true;
}

// ─────────────────────────────────────────────
// STEP / IGES Export (OCCT required)
// ─────────────────────────────────────────────

#ifdef USE_OCCT

bool Exporter3D::exportSTEP(const std::vector<Body3D*>& bodies,
                             const std::string& filepath) {
    STEPControl_Writer writer;
    Interface_Static::SetCVal("xstep.cascade.unit", "MM");
    Interface_Static::SetCVal("write.step.schema", "AP214");

    bool hasShapes = false;
    for (const auto* body : bodies) {
        if (body && body->hasOCCTShape()) {
            // Export in world space so placement (move/rotate/scale) is kept
            IFSelect_ReturnStatus status = writer.Transfer(
                body->getWorldShape(), STEPControl_AsIs);
            if (status == IFSelect_RetDone) hasShapes = true;
        }
    }

    if (!hasShapes) {
        std::cerr << "Exporter3D: No OCCT shapes to export as STEP" << std::endl;
        return false;
    }

    IFSelect_ReturnStatus status = writer.Write(filepath.c_str());
    if (status != IFSelect_RetDone) {
        std::cerr << "Exporter3D: STEP write failed" << std::endl;
        return false;
    }

    std::cout << "Exported STEP: " << filepath << std::endl;
    return true;
}

bool Exporter3D::exportIGES(const std::vector<Body3D*>& bodies,
                             const std::string& filepath) {
    IGESControl_Writer writer("MM", 0); // millimeters, IGES 5.3

    bool hasShapes = false;
    for (const auto* body : bodies) {
        if (body && body->hasOCCTShape()) {
            // Export in world space so placement is kept
            if (writer.AddShape(body->getWorldShape())) {
                hasShapes = true;
            }
        }
    }

    if (!hasShapes) {
        std::cerr << "Exporter3D: No OCCT shapes to export as IGES" << std::endl;
        return false;
    }

    writer.ComputeModel();
    if (!writer.Write(filepath.c_str())) {
        std::cerr << "Exporter3D: IGES write failed" << std::endl;
        return false;
    }

    std::cout << "Exported IGES: " << filepath << std::endl;
    return true;
}

bool Exporter3D::exportSTEPShape(const TopoDS_Shape& shape, const std::string& filepath) {
    if (shape.IsNull()) {
        std::cerr << "Exporter3D: null shape for STEP export" << std::endl;
        return false;
    }
    STEPControl_Writer writer;
    Interface_Static::SetCVal("xstep.cascade.unit", "MM");
    Interface_Static::SetCVal("write.step.schema", "AP214");
    if (writer.Transfer(shape, STEPControl_AsIs) != IFSelect_RetDone) {
        std::cerr << "Exporter3D: STEP transfer failed" << std::endl;
        return false;
    }
    if (writer.Write(filepath.c_str()) != IFSelect_RetDone) {
        std::cerr << "Exporter3D: STEP write failed" << std::endl;
        return false;
    }
    std::cout << "Exported STEP: " << filepath << std::endl;
    return true;
}

bool Exporter3D::exportIGESShape(const TopoDS_Shape& shape, const std::string& filepath) {
    if (shape.IsNull()) {
        std::cerr << "Exporter3D: null shape for IGES export" << std::endl;
        return false;
    }
    IGESControl_Writer writer("MM", 0);
    if (!writer.AddShape(shape)) {
        std::cerr << "Exporter3D: IGES AddShape failed" << std::endl;
        return false;
    }
    writer.ComputeModel();
    if (!writer.Write(filepath.c_str())) {
        std::cerr << "Exporter3D: IGES write failed" << std::endl;
        return false;
    }
    std::cout << "Exported IGES: " << filepath << std::endl;
    return true;
}

std::vector<std::unique_ptr<Body3D>> Exporter3D::importSTEP(const std::string& filepath) {
    std::vector<std::unique_ptr<Body3D>> result;

    STEPControl_Reader reader;
    IFSelect_ReturnStatus status = reader.ReadFile(filepath.c_str());
    if (status != IFSelect_RetDone) {
        std::cerr << "Exporter3D: Cannot read STEP file: " << filepath << std::endl;
        return result;
    }

    reader.TransferRoots();
    int nbShapes = reader.NbShapes();

    for (int i = 1; i <= nbShapes; ++i) {
        TopoDS_Shape shape = reader.Shape(i);

        auto body = std::make_unique<Body3D>();
        body->setName("Imported_" + std::to_string(i));
        body->setKind("Import");
        body->setOCCTShape(shape);

        // Tessellate for display
        BRepMesh_IncrementalMesh mesh(shape, 0.5);
        mesh.Perform();

        // Extract mesh from OCCT shape — reuse the tessellation code path
        // by calling the private tessellateFromOCCT indirectly via setOCCTShape
        // The body already has the shape set, so we call uploadToGPU later

        result.push_back(std::move(body));
    }

    std::cout << "Imported " << result.size() << " shapes from STEP: " << filepath << std::endl;
    return result;
}

#endif // USE_OCCT

} // namespace Modeling3D
