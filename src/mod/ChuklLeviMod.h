// SDK glue for the LeviLaunchroid / Preloader side.
// UNVERIFIED: written from the public Camera Overhaul skeleton, not compiled against the real SDK.
// Every line marked SDK-CHECK must be compared with include/pl/*.hpp from LiteLDev/preloader-android.
#pragma once
#ifdef CHUKL_WITH_SDK
#include "core/Freecam.h"
#include "core/FreeLook.h"
#include "core/FullBright.h"

namespace chukl {

class ChuklLeviMod {
public:
    static ChuklLeviMod& get();   // SDK-CHECK: PL_REGISTER_MOD(TYPE, INSTANCE_EXPR) needs an instance expression
    bool load();
    bool enable();
    bool disable();
    bool unload();

private:
    void registerMenu();    // Mod Menu toggles/sliders for every CHUKL setting
    void installHooks();    // TODO: game hooks, need offsets for your Minecraft version
    void removeHooks();

    Freecam freecam_;       FreecamConfig freecamCfg_;
    FreeLook freeLook_;     FreeLookConfig freeLookCfg_;
    FullBright fullBright_;
};

}  // namespace chukl
#endif
