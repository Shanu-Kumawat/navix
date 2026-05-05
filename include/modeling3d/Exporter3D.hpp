#pragma once

#include <string>
#include <vector>
#include "modeling3d/Body3D.hpp"

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

#ifdef USE_OCCT
    // STEP export (requires OCCT B-Rep shapes)
    static bool exportSTEP(const std::vector<Body3D*>& bodies,
                           const std::string& filepath);

    // IGES export (requires OCCT B-Rep shapes)
    static bool exportIGES(const std::vector<Body3D*>& bodies,
                           const std::string& filepath);

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
