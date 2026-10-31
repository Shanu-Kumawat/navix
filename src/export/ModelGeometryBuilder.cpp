#include "export/ModelGeometryBuilder.hpp"
#include "shapes/BasicShapes.hpp"
#include "shapes/ComplexShapes.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>

#ifdef USE_OCCT
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <Standard_Failure.hxx>
#endif

namespace Export {

#ifdef USE_OCCT

namespace {

// Build a closed planar wire from 2D profile points and revolve it 360°.
// axisDir: X axis (points = axial, radius) or Z axis (points = radius, axial).
enum class RevolveAxis { X, Z };

TopoDS_Solid revolveClosedProfile(const std::vector<glm::dvec2>& pts, RevolveAxis axis) {
    if (pts.size() < 3) return TopoDS_Solid();

    auto toPnt = [&](const glm::dvec2& p) -> gp_Pnt {
        if (axis == RevolveAxis::X) return gp_Pnt(p.x, p.y, 0.0);   // (axial, radius)
        return gp_Pnt(p.x, 0.0, p.y);                               // (radius, axial)
    };

    BRepBuilderAPI_MakeWire wire;
    gp_Pnt prev = toPnt(pts.back());
    for (const auto& p : pts) {
        gp_Pnt cur = toPnt(p);
        if (prev.Distance(cur) < 1e-9) continue;
        wire.Add(BRepBuilderAPI_MakeEdge(prev, cur));
        prev = cur;
    }
    if (!wire.IsDone()) return TopoDS_Solid();

    TopoDS_Face face = BRepBuilderAPI_MakeFace(wire.Wire());
    gp_Ax1 revolAxis(gp_Pnt(0, 0, 0),
                     axis == RevolveAxis::X ? gp_Dir(1, 0, 0) : gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeRevol revol(face, revolAxis);
    if (!revol.IsDone()) return TopoDS_Solid();

    return TopoDS::Solid(revol.Shape());
}

// Add a shape to a compound (shallow copy into the compound's builder)
void addToCompound(TopoDS_Compound& comp, BRep_Builder& builder, const TopoDS_Shape& s) {
    if (!s.IsNull()) builder.Add(comp, s);
}

} // anonymous namespace

TopoDS_Compound ModelGeometryBuilder::buildBellows(const Drawing::Bellows& bellows) {
    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);

    try {
        const std::vector<glm::dvec2>& profile = bellows.getCachedProfile();
        if (profile.size() < 2) {
            std::cerr << "[ModelGeometryBuilder] bellows profile is empty" << std::endl;
            return comp;
        }
        const double t = std::max<double>(bellows.wallThickness, 0.5);

        // Closed wall cross-section: outer path forward, inner path (offset by
        // wall thickness) back. Profile is (x=axial, y=radius) in mm.
        std::vector<glm::dvec2> section;
        for (const auto& p : profile)
            section.push_back(glm::dvec2(p.x, p.y));
        for (auto it = profile.rbegin(); it != profile.rend(); ++it)
            section.push_back(glm::dvec2(it->x, std::max(it->y - t, 0.5)));

        TopoDS_Solid solid = revolveClosedProfile(section, RevolveAxis::X);
        if (solid.IsNull()) {
            std::cerr << "[ModelGeometryBuilder] bellows revolve failed" << std::endl;
            return comp;
        }
        addToCompound(comp, builder, solid);
    } catch (const Standard_Failure& e) {
        std::cerr << "[ModelGeometryBuilder] bellows failed: " << e.GetMessageString() << std::endl;
    } catch (...) {
        std::cerr << "[ModelGeometryBuilder] bellows failed (unknown)" << std::endl;
    }
    return comp;
}

TopoDS_Compound ModelGeometryBuilder::buildBallBearing(const Drawing::BallBearing& bearing) {
    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);

    try {
        const double outerR = bearing.outerDiameter / 2.0;
        const double innerR = bearing.innerDiameter / 2.0;
        const double halfW  = bearing.width / 2.0;
        const double raceWidth   = (outerR - innerR) * 0.18;
        const double innerRaceOuter = innerR + raceWidth;
        const double outerRaceInner = outerR - raceWidth;
        const double raceHeight  = halfW * 0.7;
        const double grooveDepth = raceWidth * 0.15;
        const double h = raceHeight, g = grooveDepth;

        // Inner race ring section (radius, axial) — closed loop, revolved
        // around the bearing axis (Z)
        std::vector<glm::dvec2> inner = {
            { innerR, -h }, { innerRaceOuter - g, -h },
            { (innerR + innerRaceOuter) / 2.0, -h * 0.8 },
            { (innerR + innerRaceOuter) / 2.0, 0.0 },
            { (innerR + innerRaceOuter) / 2.0, h * 0.8 },
            { innerRaceOuter - g, h }, { innerR, h },
        };
        TopoDS_Solid innerRace = revolveClosedProfile(inner, RevolveAxis::Z);
        addToCompound(comp, builder, innerRace);

        // Outer race ring section
        std::vector<glm::dvec2> outer = {
            { outerRaceInner + g, -h },
            { (outerRaceInner + outerR) / 2.0, -h * 0.8 },
            { (outerRaceInner + outerR) / 2.0, 0.0 },
            { (outerRaceInner + outerR) / 2.0, h * 0.8 },
            { outerRaceInner + g, h }, { outerR, h }, { outerR, -h },
        };
        TopoDS_Solid outerRace = revolveClosedProfile(outer, RevolveAxis::Z);
        addToCompound(comp, builder, outerRace);

        // Balls on the pitch circle (Z = 0); pitch diameter = (OD + ID) / 2
        const double pitchR = (bearing.outerDiameter + bearing.innerDiameter) / 4.0;
        const double ballR = bearing.ballDiameter / 2.0;
        for (int i = 0; i < bearing.numBalls; ++i) {
            double ang = (2.0 * M_PI * i) / bearing.numBalls;
            gp_Pnt center(pitchR * std::cos(ang), pitchR * std::sin(ang), 0.0);
            BRepPrimAPI_MakeSphere sphere(center, ballR);
            // NOTE: MakeSphere::IsDone() is unreliable (returns false for
            // valid spheres) — check the shape instead
            if (!sphere.Shape().IsNull()) addToCompound(comp, builder, sphere.Shape());
        }
    } catch (const Standard_Failure& e) {
        std::cerr << "[ModelGeometryBuilder] bearing failed: " << e.GetMessageString() << std::endl;
    } catch (...) {
        std::cerr << "[ModelGeometryBuilder] bearing failed (unknown)" << std::endl;
    }
    return comp;
}

TopoDS_Compound ModelGeometryBuilder::buildShockAbsorber(
        const Drawing::Spring2D* spring,
        const Drawing::ShockAbsorberEnd2D* /*end*/,
        const Drawing::ShockAbsorberBottomEnd* /*bottomEnd*/) {
    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);

    if (!spring) {
        std::cerr << "[ModelGeometryBuilder] no spring for shock absorber" << std::endl;
        return comp;
    }

    try {
        // Same proportions as ShockAbsorberModel3D::generateFEMMesh, in mm
        const double springOuterRadius = spring->outerDiameter / 2.0;
        const double springWireRadius  = spring->wireDiameter / 2.0;
        const double springHelixRadius = springOuterRadius - springWireRadius;
        const double springLength      = spring->freeLength;
        const int    numCoils          = std::max(1, spring->numCoils);

        const double pistonRodRadius       = springWireRadius * 0.8;
        const double damperTubeOuterRadius = springHelixRadius * 0.35;
        const double damperTubeInnerRadius = damperTubeOuterRadius * 0.7;

        const double springStartY = -springLength * 0.5;
        const double springEndY   = springStartY + springLength;

        const double topNutHeight    = springLength * 0.08;
        const double topCapHeight    = springLength * 0.06;
        const double topCollarHeight = springLength * 0.05;
        const double upperSeatThickness = springLength * 0.035;
        const double lowerSeatThickness = springLength * 0.035;
        const double lowerSeatRadius = springOuterRadius * 1.05;
        const double upperSeatRadius = springOuterRadius * 1.05;
        const double topCollarRadius = springOuterRadius * 0.5;
        const double topNutRadius    = springOuterRadius * 0.4;
        const double topCapRadius    = springOuterRadius * 0.35;
        const double bottomMountHeight  = springLength * 0.35;
        const double bottomPlateThickness = springWireRadius * 0.7;
        const double discHeight = springLength * 0.04;
        const double discRadius = lowerSeatRadius * 1.3;

        // Solid cylinder along Y from bottomY to topY
        auto mkCylY = [&](double bottomY, double topY, double r) -> TopoDS_Shape {
            double height = topY - bottomY;
            if (height <= 0 || r <= 0) return TopoDS_Shape();
            return BRepPrimAPI_MakeCylinder(
                gp_Ax2(gp_Pnt(0, bottomY, 0), gp_Dir(0, 1, 0)), r, height).Shape();
        };
        // Annular tube: outer minus inner
        auto mkTubeY = [&](double bottomY, double topY, double rOut, double rIn) -> TopoDS_Shape {
            TopoDS_Shape o = mkCylY(bottomY, topY, rOut);
            TopoDS_Shape i = mkCylY(bottomY, topY, rIn);
            if (o.IsNull() || i.IsNull()) return TopoDS_Shape();
            BRepAlgoAPI_Cut cut(o, i);
            if (!cut.IsDone()) return TopoDS_Shape();
            return cut.Shape();
        };

        // 1. Lower spring seat (annular)
        addToCompound(comp, builder, mkTubeY(springStartY - lowerSeatThickness, springStartY,
                                             lowerSeatRadius, damperTubeOuterRadius));
        // 2. Upper damper tube (hollow)
        double upperTubeBottom = springStartY + springLength * 0.3;
        double upperTubeTop = springEndY + upperSeatThickness + topCollarHeight + topNutHeight + topCapHeight;
        addToCompound(comp, builder, mkTubeY(upperTubeBottom, upperTubeTop,
                                             damperTubeOuterRadius, damperTubeInnerRadius));
        // 3. Lower piston rod (solid)
        double lowerRodTop = springEndY - springLength * 0.3;
        double lowerRodBottom = springStartY - lowerSeatThickness - bottomMountHeight - bottomPlateThickness;
        addToCompound(comp, builder, mkCylY(lowerRodBottom, lowerRodTop, pistonRodRadius));
        // 4. Coil spring — one torus per coil (axis along Y)
        {
            double coilPitch = springLength / numCoils;
            for (int c = 0; c < numCoils; ++c) {
                double centerY = springStartY + (c + 0.5) * coilPitch;
                BRepPrimAPI_MakeTorus torus(
                    gp_Ax2(gp_Pnt(0, centerY, 0), gp_Dir(0, 1, 0)),
                    springHelixRadius, springWireRadius);
                if (torus.IsDone()) addToCompound(comp, builder, torus.Shape());
            }
        }
        // 5. Upper spring seat (annular)
        addToCompound(comp, builder, mkTubeY(springEndY, springEndY + upperSeatThickness,
                                             upperSeatRadius, pistonRodRadius));
        // 6. Top collar (hollow)
        double collarBottom = springEndY + upperSeatThickness;
        double collarTop = collarBottom + topCollarHeight;
        addToCompound(comp, builder, mkTubeY(collarBottom, collarTop, topCollarRadius, pistonRodRadius));
        // 7. Top nut (solid)
        double nutBottom = collarTop;
        addToCompound(comp, builder, mkCylY(nutBottom, nutBottom + topNutHeight, topNutRadius));
        // 8. Top cap (solid)
        double capBottom = nutBottom + topNutHeight;
        addToCompound(comp, builder, mkCylY(capBottom, capBottom + topCapHeight, topCapRadius));
        // 9. Bottom disc (annular)
        double discTop = springStartY - lowerSeatThickness;
        addToCompound(comp, builder, mkTubeY(discTop - discHeight, discTop, discRadius, pistonRodRadius));
        // 10. Bottom U-mount legs (2 boxes)
        double uMountTop = discTop - discHeight;
        double halfW = springOuterRadius * 0.45;
        double legWidth = springWireRadius * 1.2;
        double legDepth = springWireRadius * 0.9;
        double legBottom = uMountTop - bottomMountHeight;
        addToCompound(comp, builder,
            BRepPrimAPI_MakeBox(gp_Pnt(-halfW, legBottom, -legDepth),
                                legWidth, uMountTop - legBottom, 2 * legDepth).Shape());
        addToCompound(comp, builder,
            BRepPrimAPI_MakeBox(gp_Pnt(halfW - legWidth, legBottom, -legDepth),
                                legWidth, uMountTop - legBottom, 2 * legDepth).Shape());
    } catch (const Standard_Failure& e) {
        std::cerr << "[ModelGeometryBuilder] shock absorber failed: " << e.GetMessageString() << std::endl;
    } catch (...) {
        std::cerr << "[ModelGeometryBuilder] shock absorber failed (unknown)" << std::endl;
    }
    return comp;
}

#endif // USE_OCCT

} // namespace Export
