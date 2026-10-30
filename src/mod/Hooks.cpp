#ifdef CHUKL_WITH_SDK
#include "mod/Hooks.h"

#include <atomic>
#include <mutex>

#include <pl/Mod.hpp>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include "mod/ChuklLeviMod.h"

namespace chukl::hooks {
namespace {

constexpr const char* kModule = "libminecraftpe.so";

// =====================================================================================
// TODO(RE): fill these with byte signatures for YOUR Minecraft version.
// An empty string means "skip this hook": the mod still loads and the menu still works.
// Find them by disassembling libminecraftpe.so (or from the RTTI/vtable approach used by
// Camera Overhaul), then pass the pattern as accepted by pl::memory::resolveSignature.
// =====================================================================================
constexpr const char* kSigCameraPos = "";   // camera position getter
constexpr const char* kSigLookInput = "";   // mouse / touch look handler
constexpr const char* kSigBrightness = "";  // options brightness (gamma) getter

// ---- Prototypes are ASSUMED. Verify each one against the disassembly before shipping. ----
using CameraPosFn = void (*)(void* self, float* outXyz);
using LookFn = void (*)(void* self, float dx, float dy);
using BrightnessFn = float (*)(void* options);

CameraPosFn g_origCameraPos = nullptr;
LookFn g_origLook = nullptr;
BrightnessFn g_origBrightness = nullptr;

pl::memory::HookHandle g_hCameraPos, g_hLook, g_hBrightness;

std::mutex g_cacheMutex;
CameraPose g_lastPose;
bool g_havePose = false;
std::atomic<double> g_lastBrightness{1.0};
std::atomic_bool g_haveBrightness{false};

// ---------------------------------------------------------------- detours

void cameraPosDetour(void* self, float* outXyz) {
    g_origCameraPos(self, outXyz);
    {
        std::lock_guard lock(g_cacheMutex);
        g_lastPose.x = outXyz[0];
        g_lastPose.y = outXyz[1];
        g_lastPose.z = outXyz[2];
        g_havePose = true;
    }
    CameraPose p;
    if (ChuklLeviMod::get().freecamPose(/*partial tick*/ 1.0, p)) {
        outXyz[0] = static_cast<float>(p.x);
        outXyz[1] = static_cast<float>(p.y);
        outXyz[2] = static_cast<float>(p.z);
    }
}

void lookDetour(void* self, float dx, float dy) {
    if (ChuklLeviMod::get().onLook(dx, dy)) return;  // consumed: player does not rotate
    g_origLook(self, dx, dy);
}

float brightnessDetour(void* options) {
    const float real = g_origBrightness(options);
    g_lastBrightness.store(real);
    g_haveBrightness.store(true);
    double out;
    if (ChuklLeviMod::get().brightnessOverride(real, out)) return static_cast<float>(out);
    return real;
}

// ---------------------------------------------------------------- install helper

bool installOne(const char* name, const char* sig, pl::memory::FuncPtr detour,
                pl::memory::FuncPtr* orig, pl::memory::HookHandle& handle) {
    auto& log = ll::mod::NativeMod::current()->getLogger();
    if (!sig || !*sig) {
        log.warn("Hook '{}' skipped: no signature set", name);
        return false;
    }
    const uintptr_t addr = pl::memory::resolveSignature(sig, kModule);
    if (!addr) {
        log.error("Hook '{}' skipped: signature not found in {}", name, kModule);
        return false;
    }
    handle = pl::memory::HookHandle(reinterpret_cast<pl::memory::FuncPtr>(addr), detour, orig);
    if (!handle.installed()) {
        log.error("Hook '{}' failed to install", name);
        return false;
    }
    log.info("Hook '{}' installed", name);
    return true;
}

}  // namespace

bool install() {
    const bool a = installOne("cameraPos", kSigCameraPos, reinterpret_cast<pl::memory::FuncPtr>(&cameraPosDetour),
                              reinterpret_cast<pl::memory::FuncPtr*>(&g_origCameraPos), g_hCameraPos);
    const bool b = installOne("look", kSigLookInput, reinterpret_cast<pl::memory::FuncPtr>(&lookDetour),
                              reinterpret_cast<pl::memory::FuncPtr*>(&g_origLook), g_hLook);
    const bool c = installOne("brightness", kSigBrightness, reinterpret_cast<pl::memory::FuncPtr>(&brightnessDetour),
                              reinterpret_cast<pl::memory::FuncPtr*>(&g_origBrightness), g_hBrightness);
    return a && b && c;
}

void remove() {
    g_hCameraPos.reset();
    g_hLook.reset();
    g_hBrightness.reset();
    g_havePose = false;
    g_haveBrightness = false;
}

bool readPlayerPose(CameraPose& out) {
    std::lock_guard lock(g_cacheMutex);
    if (!g_havePose) return false;
    out = g_lastPose;
    return true;
}

bool readBrightness(double& out) {
    if (!g_haveBrightness.load()) return false;
    out = g_lastBrightness.load();
    return true;
}

void writeBrightness(double) {}

}  // namespace chukl::hooks
#endif
