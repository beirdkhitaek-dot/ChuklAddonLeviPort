// Platform-independent Freecam logic, mirroring CHUKL's VulxtsFreecam (Java).
// No game or SDK headers here, so it builds and tests on a normal PC.
#pragma once
#include <algorithm>
#include <cmath>

namespace chukl {

struct FreecamConfig {
    double speed = 1.0;          // 0.1 .. 50
    double verticalSpeed = 1.0;  // 0.2 .. 5
    bool smoothing = true;
    bool showOwnBody = true;
    double lookSensitivity = 0.5;  // 0.1 .. 2
    int chunkDistance = 12;        // 2 .. 32 (needs a game hook)
};

struct FreecamInput {
    bool forward = false, back = false, left = false, right = false, up = false, down = false;
};

class Freecam {
public:
    static constexpr double kScrollStep = 0.1;
    static constexpr float kLookScale = 0.15f;  // same constant as CHUKL's MouseMixin

    void begin(double x, double y, double z, float yaw, float pitch) {
        x_ = px_ = x; y_ = py_ = y; z_ = pz_ = z;
        yaw_ = yaw; pitch_ = pitch;
        detached_ = true;
    }
    void end() { detached_ = false; }
    bool detached() const { return detached_; }

    // One game tick of camera movement, same formula as the Java version.
    void tick(const FreecamInput& in, const FreecamConfig& cfg) {
        if (!detached_) return;
        px_ = x_; py_ = y_; pz_ = z_;

        const double spd = cfg.speed;
        const double ease = cfg.smoothing ? 0.5 : 1.0;

        double forward = 0, side = 0, up = 0;
        if (in.forward) forward++;
        if (in.back) forward--;
        if (in.left) side++;
        if (in.right) side--;
        if (in.up) up++;
        if (in.down) up--;

        const double yawRad = yaw_ * M_PI / 180.0;
        const double dx = -std::sin(yawRad) * forward * spd + std::cos(yawRad) * side * spd;
        const double dz = std::cos(yawRad) * forward * spd + std::sin(yawRad) * side * spd;
        const double dy = up * spd * cfg.verticalSpeed;

        x_ += dx * ease;
        y_ += dy * ease;
        z_ += dz * ease;
    }

    void addLook(double dx, double dy, const FreecamConfig& cfg) {
        if (!detached_) return;
        yaw_ += static_cast<float>(dx * cfg.lookSensitivity) * kLookScale;
        pitch_ = std::clamp(pitch_ + static_cast<float>(dy * cfg.lookSensitivity) * kLookScale, -90.0f, 90.0f);
    }

    // Scroll wheel changes fly speed; returns true if the scroll was consumed.
    bool adjustSpeedFromScroll(double scroll, FreecamConfig& cfg) const {
        if (!detached_ || scroll == 0.0) return false;
        cfg.speed = std::clamp(cfg.speed + std::copysign(kScrollStep, scroll), 0.1, 50.0);
        return true;
    }

    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
    double lerpX(double t) const { return px_ + (x_ - px_) * t; }
    double lerpY(double t) const { return py_ + (y_ - py_) * t; }
    double lerpZ(double t) const { return pz_ + (z_ - pz_) * t; }

private:
    double x_ = 0, y_ = 0, z_ = 0, px_ = 0, py_ = 0, pz_ = 0;
    float yaw_ = 0, pitch_ = 0;
    bool detached_ = false;
};

}  // namespace chukl
