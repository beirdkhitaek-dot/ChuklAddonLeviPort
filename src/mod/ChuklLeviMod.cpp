#ifdef CHUKL_WITH_SDK
#include "mod/ChuklLeviMod.h"
// SDK-CHECK: exact header names. Camera Overhaul lists these:
#include <pl/Mod.hpp>
#include <pl/Logger.hpp>
#include <pl/ModMenu.hpp>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Vtable.hpp>

namespace chukl {

ChuklLeviMod& ChuklLeviMod::get() { static ChuklLeviMod m; return m; }

bool ChuklLeviMod::load() {
    try {
        auto& self = *ll::mod::NativeMod::current();           // SDK-CHECK
        self.getLogger().info("chukl_levi loaded");            // SDK-CHECK
        return true;
    } catch (...) { return false; }                            // never let exceptions cross the lifecycle boundary
}

bool ChuklLeviMod::enable() {
    try { registerMenu(); installHooks(); return true; } catch (...) { return false; }
}
bool ChuklLeviMod::disable() {
    try { removeHooks(); return true; } catch (...) { return false; }
}
bool ChuklLeviMod::unload() { return disable(); }

void ChuklLeviMod::registerMenu() {
    // SDK-CHECK: pl::modmenu::ModuleBuilder with ConfigType::Toggle / SliderInt / SliderFloat.
    // Register one module per feature, bound to these fields:
    //   chukl.freecam   : speed 0.1-50, vertical-speed 0.2-5, smoothing, show-own-body, look-sensitivity 0.1-2, chunk-distance 2-32
    //   chukl.freelook  : mode (Player/Camera), toggle-perspective, through-walls, camera-sensitivity 0.1-10,
    //                     arrows-control-opposite, arrow-speed 0-10
    //   chukl.fullbright: on/off
}

void ChuklLeviMod::installHooks() {
    // TODO (needs reverse engineering of libminecraftpe.so for your exact version):
    //  1. Camera position/rotation: override the camera getters (Camera Overhaul resolves CameraAPI vtable slots
    //     through RTTI, no hard-coded addresses). Feed Freecam::lerpX/Y/Z + yaw/pitch to the renderer.
    //  2. Input: swallow mouse look and scroll while freecam is active (Freecam::addLook / adjustSpeedFromScroll);
    //     latch the walking input so the body keeps moving (CHUKL's "input latches").
    //  3. Brightness: set/restore the game's brightness option (FullBright::enable/reassert/disable).
}
void ChuklLeviMod::removeHooks() { /* remove every hook installed above */ }

}  // namespace chukl
#endif
