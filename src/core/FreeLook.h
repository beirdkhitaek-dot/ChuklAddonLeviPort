// Platform-independent FreeLook logic, mirroring CHUKL's VulxtsFreeLook (Java).
#pragma once
#include <algorithm>

namespace chukl {

enum class FreeLookMode { Player, Camera };

struct FreeLookConfig {
    FreeLookMode mode = FreeLookMode::Player;  // which one the mouse rotates
    bool togglePerspective = true;
    bool throughWalls = false;
    double cameraSensitivity = 8.0;  // 0.1 .. 10, higher = slower
    bool arrowsControlOpposite = true;
    double arrowSpeed = 4.0;  // 0 .. 10
};

struct ArrowKeys { bool left = false, right = false, up = false, down = false; };
struct PlayerRotationDelta { float yaw = 0, pitch = 0; };

class FreeLook {
public:
    void begin(float playerYaw, float playerPitch) { camYaw_ = playerYaw; camPitch_ = playerPitch; active_ = true; }
    void end() { active_ = false; }
    bool active() const { return active_; }

    // In Camera mode the mouse rotates the camera (returns true = consume the event).
    bool addCameraLook(double dx, double dy, const FreeLookConfig& cfg) {
        if (!active_ || cfg.mode != FreeLookMode::Camera) return false;
        float sens = static_cast<float>(cfg.cameraSensitivity);
        if (sens <= 0.0f) sens = 1.0f;
        camYaw_ += static_cast<float>(dx) / sens;
        camPitch_ = std::clamp(camPitch_ + static_cast<float>(dy) / sens, -90.0f, 90.0f);
        return true;
    }

    // Arrow keys rotate "the other one"; returns how much to rotate the real player.
    PlayerRotationDelta tick(const ArrowKeys& k, const FreeLookConfig& cfg) {
        PlayerRotationDelta out;
        if (!active_ || !cfg.arrowsControlOpposite) return out;
        const float step = static_cast<float>(cfg.arrowSpeed * 2.0) * 0.5f;
        const float dYaw = (k.right ? step : 0.f) - (k.left ? step : 0.f);
        const float dPitch = (k.down ? step : 0.f) - (k.up ? step : 0.f);
        if (cfg.mode == FreeLookMode::Player) { camYaw_ += dYaw; camPitch_ += dPitch; }
        else { out.yaw = dYaw; out.pitch = dPitch; }
        camPitch_ = std::clamp(camPitch_, -90.0f, 90.0f);
        return out;
    }

    float cameraYaw() const { return camYaw_; }
    float cameraPitch() const { return camPitch_; }

private:
    float camYaw_ = 0, camPitch_ = 0;
    bool active_ = false;
};

}  // namespace chukl
