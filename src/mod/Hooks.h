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
// FullBright is applied inside the brightness getter detour, so there is nothing to write back.
// Kept so the mod class has one symmetrical API; a setter hook can be added later.
void writeBrightness(double value);

}  // namespace chukl::hooks
#endif
