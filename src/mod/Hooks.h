#pragma once
#ifdef CHUKL_WITH_SDK
#include "core/Pose.h"

namespace chukl::hooks {

// Resolves every signature in Hooks.cpp and installs the detours that resolved.
// Returns true only if ALL hooks installed. Unresolved hooks leave their feature inert.
bool install();
void remove();

// Last real (non-overridden) values the game reported, cached by the detours.
bool readPlayerPose(CameraPose& out);
bool readBrightness(double& out);
// Writes the gfx_gamma option object directly (both candidate float fields). No-op if not captured.
void writeBrightness(double value);

}  // namespace chukl::hooks
#endif
