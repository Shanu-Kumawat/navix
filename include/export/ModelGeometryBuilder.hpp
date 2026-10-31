#pragma once

// Builds exact OCCT B-Rep solids for the parametric models (bellows,
// ball bearing, shock absorber) so they can be exported as STEP/IGES
// with true CAD geometry (importable by ANSYS, Abaqus, FreeCAD, ...).
//
// All dimensions are in millimetres, matching the 2D design parameters.

#include <string>

#ifdef USE_OCCT
#include <TopoDS_Compound.hxx>
#endif

namespace Drawing { class Bellows; class BallBearing; class Spring2D; class ShockAbsorberEnd2D; class ShockAbsorberBottomEnd; }

namespace Export {

class ModelGeometryBuilder {
public:
#ifdef USE_OCCT
    // Hollow convoluted bellows: wall cross-section revolved around the X axis
    static TopoDS_Compound buildBellows(const Drawing::Bellows& bellows);

    // Inner race + outer race (revolved ring sections) + N balls (spheres)
    static TopoDS_Compound buildBallBearing(const Drawing::BallBearing& bearing);

    // Damper tubes, seats, collar/nut/cap, spring coils (tori), bottom mount
    static TopoDS_Compound buildShockAbsorber(const Drawing::Spring2D* spring,
                                              const Drawing::ShockAbsorberEnd2D* end,
                                              const Drawing::ShockAbsorberBottomEnd* bottomEnd);
#endif
};

} // namespace Export
