#ifdef CHUKL_WITH_SDK
#include "mod/ChuklLeviMod.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <string>

#include <pl/ModMenu.hpp>

#include "mod/Hooks.h"

namespace chukl {
namespace {

constexpr const char* kFreecamId = "chukl.freecam";
constexpr const char* kFreeLookId = "chukl.freelook";
constexpr const char* kFullBrightId = "chukl.fullbright";

bool parseBool(std::string_view v, bool fallback) {
    if (v == "true" || v == "1" || v == "on" || v == "enabled") return true;
    if (v == "false" || v == "0" || v == "off" || v == "disabled") return false;
    return fallback;
}

double parseDouble(std::string_view v, double fallback) {
    std::string s(v);
    char* end = nullptr;
    errno = 0;
    const double d = std::strtod(s.c_str(), &end);
    if (end == s.c_str() || *end != '\0' || errno == ERANGE) return fallback;
    return d;
}

std::string num(double v) {
    std::ostringstream os;
    os << v;
    return os.str();
}
std::string flag(bool b) { return b ? "true" : "false"; }

}  // namespace

ChuklLeviMod& ChuklLeviMod::get() {
    static ChuklLeviMod m;
    return m;
}

ChuklLeviMod::ChuklLeviMod() : self_(*ll::mod::NativeMod::current()) {}

// ---------------------------------------------------------------- lifecycle
// Exceptions must never cross the lifecycle boundary.

bool ChuklLeviMod::load() {
    try {
        self_.getLogger().info("chukl_levi loaded");
        return true;
    } catch (...) { return false; }
}

bool ChuklLeviMod::enable() {
    try {
        const bool menuOk = registerModules_();
        const bool hooksOk = hooks::install();
        if (!hooksOk) {
            self_.getLogger().warn("Some game hooks are unresolved; affected features stay inert");
        }
        return menuOk;  // missing hooks must not stop the menu from appearing
    } catch (...) { return false; }
}

bool ChuklLeviMod::disable() {
    try {
        {
            std::lock_guard lock(mutex_);
            freecam_.end();
            freeLook_.end();
        }
        stopFullBright_();
        hooks::remove();
        unregisterModules_();
        return true;
    } catch (...) { return false; }
}

bool ChuklLeviMod::unload() { return disable(); }

// ---------------------------------------------------------------- menu

bool ChuklLeviMod::registerModules_() {
    using namespace pl::modmenu;
    FreecamConfig fc;
    FreeLookConfig fl;
    {
        std::lock_guard lock(mutex_);
        fc = freecamCfg_;
        fl = freeLookCfg_;
    }
    auto onToggle = [](std::string_view id, bool on) { get().handleToggle_(id, on); };
    auto onCfg = [](std::string_view id, std::string_view k, std::string_view v) {
        get().handleConfigChanged_(id, k, v);
    };

    const bool a =
        ModuleBuilder(kFreecamId, "Freecam")
            .modId(self_.getId())
            .description("Detach the camera and fly around.")
            .defaultEnabled(false)
            .onToggle(onToggle)
            .config("speed", "Speed", ConfigType::SliderFloat, num(fc.speed), "0.1", "50")
            .config("verticalSpeed", "Vertical Speed", ConfigType::SliderFloat, num(fc.verticalSpeed), "0.2", "5")
            .config("smoothing", "Smoothing", ConfigType::Toggle, flag(fc.smoothing))
            .config("showOwnBody", "Show Own Body", ConfigType::Toggle, flag(fc.showOwnBody))
            .config("lookSensitivity", "Look Sensitivity", ConfigType::SliderFloat, num(fc.lookSensitivity), "0.1", "2")
            .config("chunkDistance", "Chunk Distance", ConfigType::SliderInt, std::to_string(fc.chunkDistance), "2", "32")
            .onConfigChanged(onCfg)
            .registerModule();

    const bool b =
        ModuleBuilder(kFreeLookId, "FreeLook")
            .modId(self_.getId())
            .description("Look around without turning the player (or the reverse).")
            .defaultEnabled(false)
            .onToggle(onToggle)
            .config("mode", "Mode", ConfigType::Radio,
                    std::to_string(static_cast<int>(fl.mode)), "Player,Camera")
            .config("togglePerspective", "Toggle Perspective", ConfigType::Toggle, flag(fl.togglePerspective))
            .config("throughWalls", "Camera Through Walls", ConfigType::Toggle, flag(fl.throughWalls))
            .config("cameraSensitivity", "Camera Sensitivity", ConfigType::SliderFloat, num(fl.cameraSensitivity), "0.1", "10")
            .config("arrowsControlOpposite", "Arrows Control Opposite", ConfigType::Toggle, flag(fl.arrowsControlOpposite))
            .config("arrowSpeed", "Arrow Speed", ConfigType::SliderFloat, num(fl.arrowSpeed), "0", "10")
            .onConfigChanged(onCfg)
            .registerModule();

    const bool c =
        ModuleBuilder(kFullBrightId, "FullBright")
            .modId(self_.getId())
            .description("Maximum brightness.")
            .defaultEnabled(false)
            .onToggle(onToggle)
            .registerModule();

    if (!(a && b && c)) {
        self_.getLogger().error("Mod Menu registration failed: freecam={} freelook={} fullbright={}", a, b, c);
    }
    return a && b && c;
}

void ChuklLeviMod::unregisterModules_() {
    pl::modmenu::unregisterModule(kFreecamId);
    pl::modmenu::unregisterModule(kFreeLookId);
    pl::modmenu::unregisterModule(kFullBrightId);
}

void ChuklLeviMod::handleToggle_(std::string_view id, bool on) {
    if (id == kFreecamId) { on ? startFreecam_() : stopFreecam_(); }
    else if (id == kFreeLookId) { on ? startFreeLook_() : stopFreeLook_(); }
    else if (id == kFullBrightId) { on ? startFullBright_() : stopFullBright_(); }
}

void ChuklLeviMod::handleConfigChanged_(std::string_view id, std::string_view k, std::string_view v) {
    std::lock_guard lock(mutex_);
    if (id == kFreecamId) {
        auto& c = freecamCfg_;
        if (k == "speed") c.speed = std::clamp(parseDouble(v, c.speed), 0.1, 50.0);
        else if (k == "verticalSpeed") c.verticalSpeed = std::clamp(parseDouble(v, c.verticalSpeed), 0.2, 5.0);
        else if (k == "smoothing") c.smoothing = parseBool(v, c.smoothing);
        else if (k == "showOwnBody") c.showOwnBody = parseBool(v, c.showOwnBody);
        else if (k == "lookSensitivity") c.lookSensitivity = std::clamp(parseDouble(v, c.lookSensitivity), 0.1, 2.0);
        else if (k == "chunkDistance") c.chunkDistance = std::clamp(static_cast<int>(parseDouble(v, c.chunkDistance)), 2, 32);
    } else if (id == kFreeLookId) {
        auto& c = freeLookCfg_;
        if (k == "mode") {
            const int m = static_cast<int>(parseDouble(v, static_cast<int>(c.mode)));
            c.mode = (m == 1 || v == "Camera") ? FreeLookMode::Camera : FreeLookMode::Player;
        }
        else if (k == "togglePerspective") c.togglePerspective = parseBool(v, c.togglePerspective);
        else if (k == "throughWalls") c.throughWalls = parseBool(v, c.throughWalls);
        else if (k == "cameraSensitivity") c.cameraSensitivity = std::clamp(parseDouble(v, c.cameraSensitivity), 0.1, 10.0);
        else if (k == "arrowsControlOpposite") c.arrowsControlOpposite = parseBool(v, c.arrowsControlOpposite);
        else if (k == "arrowSpeed") c.arrowSpeed = std::clamp(parseDouble(v, c.arrowSpeed), 0.0, 10.0);
    }
}

// ---------------------------------------------------------------- feature state

void ChuklLeviMod::startFreecam_() {
    CameraPose p;
    if (!hooks::readPlayerPose(p)) {
        self_.getLogger().warn("Freecam: player pose hook unresolved, staying attached");
        return;
    }
    std::lock_guard lock(mutex_);
    freecam_.begin(p.x, p.y, p.z, p.yaw, p.pitch);
}

void ChuklLeviMod::stopFreecam_() {
    std::lock_guard lock(mutex_);
    freecam_.end();
}

void ChuklLeviMod::startFreeLook_() {
    CameraPose p;
    if (!hooks::readPlayerPose(p)) {
        self_.getLogger().warn("FreeLook: player pose hook unresolved, staying inactive");
        return;
    }
    std::lock_guard lock(mutex_);
    freeLook_.begin(p.yaw, p.pitch);
}

void ChuklLeviMod::stopFreeLook_() {
    std::lock_guard lock(mutex_);
    freeLook_.end();
}

void ChuklLeviMod::startFullBright_() {
    double current = 1.0;
    if (!hooks::readBrightness(current)) {
        self_.getLogger().warn("FullBright: brightness hook unresolved, staying off");
        return;
    }
    double target;
    {
        std::lock_guard lock(mutex_);
        target = fullBright_.enable(current);
    }
    hooks::writeBrightness(target);
}

void ChuklLeviMod::stopFullBright_() {
    double restore;
    bool wasApplied;
    {
        std::lock_guard lock(mutex_);
        wasApplied = fullBright_.applied();
        restore = fullBright_.disable();
    }
    if (wasApplied) hooks::writeBrightness(restore);
}

// ---------------------------------------------------------------- hook entry points

bool ChuklLeviMod::freecamPose(double t, CameraPose& out) {
    std::lock_guard lock(mutex_);
    if (!freecam_.detached()) return false;
    out = {freecam_.lerpX(t), freecam_.lerpY(t), freecam_.lerpZ(t), freecam_.yaw(), freecam_.pitch()};
    return true;
}

bool ChuklLeviMod::onLook(double dx, double dy) {
    std::lock_guard lock(mutex_);
    if (freecam_.detached()) { freecam_.addLook(dx, dy, freecamCfg_); return true; }
    return freeLook_.addCameraLook(dx, dy, freeLookCfg_);
}

bool ChuklLeviMod::onScroll(double scroll) {
    std::lock_guard lock(mutex_);
    return freecam_.adjustSpeedFromScroll(scroll, freecamCfg_);
}

PlayerRotationDelta ChuklLeviMod::onTick(const FreecamInput& move, const ArrowKeys& arrows) {
    std::lock_guard lock(mutex_);
    freecam_.tick(move, freecamCfg_);
    return freeLook_.tick(arrows, freeLookCfg_);
}

bool ChuklLeviMod::freeLookAngles(float& yaw, float& pitch) {
    std::lock_guard lock(mutex_);
    if (!freeLook_.active()) return false;
    yaw = freeLook_.cameraYaw();
    pitch = freeLook_.cameraPitch();
    return true;
}

bool ChuklLeviMod::brightnessOverride(double current, double& out) {
    std::lock_guard lock(mutex_);
    return fullBright_.reassert(current, out);
}

}  // namespace chukl
#endif
