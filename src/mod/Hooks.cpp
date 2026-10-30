#ifdef CHUKL_WITH_SDK
#include "mod/Hooks.h"

#include <atomic>
#include <cmath>
#include <cstddef>
#include <mutex>

#include <pl/Mod.hpp>
#include <pl/memory/Hook.hpp>
#include <pl/memory/Signature.hpp>

#include "mod/ChuklLeviMod.h"

namespace chukl::hooks {
namespace {

constexpr const char* kModule = "libminecraftpe.so";

// =====================================================================================
// Signatures are for Minecraft Bedrock 1.26.60.4 (arm64-v8a). Empty = hook skipped.
// =====================================================================================

// FloatOption constructor (libminecraftpe.so @ 0xcf966b8 in 1.26.60.4).
// Verified unique in the whole library. Every float option (gfx_gamma included) is built by it.
constexpr const char* kSigFloatOptionCtor =
    "FF C3 02 D1 EB 2B 04 6D E9 23 05 6D FD 7B 06 A9 F9 3B 00 F9 F8 5F 08 A9 F6 57 09 A9 "
    "F4 4F 0A A9 FD 83 01 91 59 D0 3B D5 F3 03 00 AA E0 23 00 91 28 17 40 F9 6B 40 20 1E "
    "48 40 20 1E 29 40 20 1E 0A 40 20 1E";

// TODO(RE): Freecam / FreeLook still need their own signatures.
constexpr const char* kSigCameraPos = "";
constexpr const char* kSigLookInput = "";

// ---- gfx_gamma facts, read from the disassembly of 1.26.60.4 ----
// Registered with: option id 50, default 0.5, min 0.0, max 1.0, step 0.001.
// Object layout written by the constructor: +0x10 min, +0x14 max, +0x18 and +0x1c = clamped default,
// +0x20 step. Which of +0x18/+0x1c the game reads is not confirmed yet, so we write both.
constexpr int kGammaOptionId = 50;
constexpr std::ptrdiff_t kValueA = 0x18;
constexpr std::ptrdiff_t kValueB = 0x1c;

// Constructor argument layout (AAPCS64): x0 self, w1 id, w2, w3, s0..s3 floats, x4, x5 strings.
using FloatOptionCtorFn = void (*)(void* self, int id, int a, int b, float def, float mn, float mx,
                                   float step, const void* key, const void* name);
// ---- ASSUMED prototypes below; verify before filling in the signatures. ----
using CameraPosFn = void (*)(void* self, float* outXyz);
using LookFn = void (*)(void* self, float dx, float dy);

FloatOptionCtorFn g_origFloatOptionCtor = nullptr;
CameraPosFn g_origCameraPos = nullptr;
LookFn g_origLook = nullptr;

pl::memory::HookHandle g_hFloatOptionCtor, g_hCameraPos, g_hLook;

std::mutex g_cacheMutex;
CameraPose g_lastPose;
bool g_havePose = false;
std::atomic<char*> g_gammaObj{nullptr};

// ---------------------------------------------------------------- detours

void floatOptionCtorDetour(void* self, int id, int a, int b, float def, float mn, float mx,
                           float step, const void* key, const void* name) {
    g_origFloatOptionCtor(self, id, a, b, def, mn, mx, step, key, name);
    // id 50 plus the exact default/min/max makes a false positive very unlikely.
    if (id == kGammaOptionId && std::fabs(def - 0.5f) < 1e-6f && mn == 0.0f &&
        std::fabs(mx - 1.0f) < 1e-6f) {
        g_gammaObj.store(static_cast<char*>(self));
        ll::mod::NativeMod::current()->getLogger().info("gfx_gamma option captured");
    }
}

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
    const bool a = installOne("floatOptionCtor", kSigFloatOptionCtor,
                              reinterpret_cast<pl::memory::FuncPtr>(&floatOptionCtorDetour),
                              reinterpret_cast<pl::memory::FuncPtr*>(&g_origFloatOptionCtor), g_hFloatOptionCtor);
    const bool b = installOne("cameraPos", kSigCameraPos, reinterpret_cast<pl::memory::FuncPtr>(&cameraPosDetour),
                              reinterpret_cast<pl::memory::FuncPtr*>(&g_origCameraPos), g_hCameraPos);
    const bool c = installOne("look", kSigLookInput, reinterpret_cast<pl::memory::FuncPtr>(&lookDetour),
                              reinterpret_cast<pl::memory::FuncPtr*>(&g_origLook), g_hLook);
    return a && b && c;
}

void remove() {
    g_hFloatOptionCtor.reset();
    g_hCameraPos.reset();
    g_hLook.reset();
    g_havePose = false;
    g_gammaObj.store(nullptr);
}

bool readPlayerPose(CameraPose& out) {
    std::lock_guard lock(g_cacheMutex);
    if (!g_havePose) return false;
    out = g_lastPose;
    return true;
}

bool readBrightness(double& out) {
    char* p = g_gammaObj.load();
    if (!p) return false;
    out = *reinterpret_cast<volatile float*>(p + kValueA);
    return true;
}

void writeBrightness(double value) {
    char* p = g_gammaObj.load();
    if (!p) return;
    const float f = static_cast<float>(value);
    *reinterpret_cast<volatile float*>(p + kValueA) = f;
    *reinterpret_cast<volatile float*>(p + kValueB) = f;
}

}  // namespace chukl::hooks
#endif
