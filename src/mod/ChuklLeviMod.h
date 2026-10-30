// SDK glue for LeviLaunchroid / preloader-android.
// API usage follows LiteLDev/LeviLaunchroid examples/full-cpp-mod (pl/Mod.hpp, pl/ModMenu.hpp).
// Not yet compiled against the NDK: build it once and fix any header drift.
#pragma once
#ifdef CHUKL_WITH_SDK
#include <mutex>
#include <string_view>

#include <pl/Mod.hpp>

#include "core/Freecam.h"
#include "core/FreeLook.h"
#include "core/FullBright.h"
#include "core/Pose.h"

namespace chukl {

class ChuklLeviMod {
public:
    static ChuklLeviMod& get();

    bool load();
    bool enable();
    bool disable();
    bool unload();

    // ---- Entry points for game hooks. Thread-safe; cheap when the feature is off. ----

    // Freecam: fills `out` with the interpolated camera pose (t = partial tick). False = not detached.
    bool freecamPose(double t, CameraPose& out);
    // Mouse/touch look delta. True = consumed (game must not rotate the player).
    bool onLook(double dx, double dy);
    // Scroll wheel. True = consumed.
    bool onScroll(double scroll);
    // Call once per game tick with the real input state. Returns how far to rotate the real player.
    PlayerRotationDelta onTick(const FreecamInput& move, const ArrowKeys& arrows);
    // FreeLook camera angles. False = FreeLook inactive.
    bool freeLookAngles(float& yaw, float& pitch);
    // FullBright: given the game's current brightness, returns true and sets `out` if it must be overridden.
    bool brightnessOverride(double current, double& out);

private:
    ChuklLeviMod();

    bool registerModules_();
    void unregisterModules_();

    void handleToggle_(std::string_view moduleId, bool enabled);
    void handleConfigChanged_(std::string_view moduleId, std::string_view key, std::string_view value);

    void startFreecam_();
    void stopFreecam_();
    void startFreeLook_();
    void stopFreeLook_();
    void startFullBright_();
    void stopFullBright_();

    ll::mod::NativeMod& self_;
    std::mutex mutex_;
    Freecam freecam_;      FreecamConfig freecamCfg_;
    FreeLook freeLook_;    FreeLookConfig freeLookCfg_;
    FullBright fullBright_;
};

}  // namespace chukl
#endif
