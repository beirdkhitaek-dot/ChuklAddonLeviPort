// FullBright bookkeeping, mirroring CHUKL's FullBright (Java): remember the player's
// value, force a high one, keep re-applying it, restore on disable.
#pragma once
#include <cmath>

namespace chukl {

class FullBright {
public:
    static constexpr double kTarget = 10.0;  // CHUKL uses gamma 10 on Java

    double enable(double currentValue) {
        previous_ = (std::fabs(currentValue - kTarget) < 1e-4) ? 1.0 : currentValue;
        applied_ = true;
        return kTarget;
    }
    bool reassert(double currentValue, double& out) const {
        if (!applied_ || std::fabs(currentValue - kTarget) <= 1e-4) return false;
        out = kTarget;
        return true;
    }
    double disable() {
        const double v = applied_ ? previous_ : 1.0;
        applied_ = false;
        return v;
    }
    bool applied() const { return applied_; }

private:
    double previous_ = 1.0;
    bool applied_ = false;
};

}  // namespace chukl
