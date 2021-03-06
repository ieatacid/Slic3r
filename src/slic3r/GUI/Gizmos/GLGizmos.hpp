#ifndef slic3r_GLGizmos_hpp_
#define slic3r_GLGizmos_hpp_

// this describes events being passed from GLCanvas3D to SlaSupport gizmo
enum class SLAGizmoEventType {
    LeftDown = 1,
    LeftUp,
    RightDown,
    Dragging,
    Delete,
    SelectAll,
    ShiftUp,
    AltUp,
    ApplyChanges,
    DiscardChanges,
    AutomaticGeneration,
    ManualEditing
};

#include "slic3r/GUI/Gizmos/GLGizmoMove.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoScale.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoRotate.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoFlatten.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoSlaSupports.hpp"
#include "slic3r/GUI/Gizmos/GLGizmoCut.hpp"

#endif //slic3r_GLGizmos_hpp_
