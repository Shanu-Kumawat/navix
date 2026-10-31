#pragma once

#include <string>
#include <vector>
#include "modeling3d/Body3D.hpp"

#ifdef USE_OCCT
#include <TopoDS_Shape.hxx>
#endif

namespace Modeling3D {

/**
 * @brief Exports 3D bodies to standard CAD interchange formats.
 *
 * Supports:
 *  - STL (ASCII & Binary) — triangle mesh export for 3D printing / FEA
 *  - STEP (AP214/AP203) — B-Rep exchange for full CAD interop (OCCT required)
 *  - IGES — legacy CAD interchange (OCCT required)
 *  - OBJ — Wavefront OBJ for visualization tools
 */
class Exporter3D {
public:
    Exporter3D() = default;
    ~Exporter3D() = default;

    // STL export (works with or without OCCT — uses display mesh)
    static bool exportSTL(const std::vector<Body3D*>& bodies,
                          const std::string& filepath,
                          bool binary = true);

    // Single body STL
    static bool exportSTL(const Body3D& body,
                          const std::string& filepath,
                          bool binary = true);

    // OBJ export (mesh-based, always available)
    static bool exportOBJ(const std::vector<Body3D*>& bodies,
                          const std::string& filepath);

    // Raw indexed-mesh export for Base3DModel display meshes.
    // vertexData layout: [x,y,z, nx,ny,nz] per vertex (stride 6 floats).
    static bool exportSTL(const std::vector<float>& vertexData,
                          const std::vector<unsigned int>& indices,
                          const std::string& filepath,
                          const std::string& name = "model",
                          bool binary = true);
    static bool exportOBJ(const std::vector<float>& vertexData,
                          const std::vector<unsigned int>& indices,
                          const std::string& filepath,
                          const std::string& name = "model");

#ifdef USE_OCCT
    // STEP export (requires OCCT B-Rep shapes)
    static bool exportSTEP(const std::vector<Body3D*>& bodies,
                           const std::string& filepath);

    // IGES export (requires OCCT B-Rep shapes)
    static bool exportIGES(const std::vector<Body3D*>& bodies,
                           const std::string& filepath);

    // STEP/IGES export of an arbitrary OCCT shape (e.g. a compound built by
    // ModelGeometryBuilder for the parametric models)
    static bool exportSTEPShape(const TopoDS_Shape& shape, const std::string& filepath);
    static bool exportIGESShape(const TopoDS_Shape& shape, const std::string& filepath);

    // STEP import — returns new bodies
    static std::vector<std::unique_ptr<Body3D>> importSTEP(const std::string& filepath);
#endif

private:
    // STL helpers
    static void writeSTLAscii(const Body3D& body, std::ostream& out);
    static void writeSTLBinary(const Body3D& body, std::ostream& out,
                               uint32_t& triangleCount);
};

} // namespace Modeling3D
