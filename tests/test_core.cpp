#include <cassert>
#include <cmath>
#include <cstdio>
#include "../src/core/Freecam.h"
#include "../src/core/FreeLook.h"
#include "../src/core/FullBright.h"
using namespace chukl;

static bool near(double a, double b, double e = 1e-6) { return std::fabs(a - b) < e; }

int main() {
    { Freecam f; FreecamConfig c; FreecamInput in; in.forward = true;
      f.begin(0, 64, 0, 0, 0); f.tick(in, c);
      assert(near(f.z(), 0.5)); assert(near(f.x(), 0)); assert(near(f.y(), 64));
      c.smoothing = false; f.tick(in, c); assert(near(f.z(), 1.5)); }
    { Freecam f; FreecamConfig c; c.smoothing = false; FreecamInput in; in.forward = true;
      f.begin(0, 0, 0, 90, 0); f.tick(in, c); assert(near(f.x(), -1.0)); assert(near(f.z(), 0.0, 1e-9)); }
    { Freecam f; FreecamConfig c; c.smoothing = false; c.speed = 2; c.verticalSpeed = 3;
      FreecamInput in; in.up = true; in.left = true; f.begin(0, 0, 0, 0, 0); f.tick(in, c);
      assert(near(f.y(), 6.0)); assert(near(f.x(), 2.0)); }
    { Freecam f; FreecamConfig c; assert(!f.adjustSpeedFromScroll(1, c));
      f.begin(0,0,0,0,0); assert(f.adjustSpeedFromScroll(1, c)); assert(near(c.speed, 1.1));
      f.adjustSpeedFromScroll(-1, c); f.adjustSpeedFromScroll(-1, c); assert(near(c.speed, 0.9));
      c.speed = 0.1; f.adjustSpeedFromScroll(-1, c); assert(near(c.speed, 0.1));
      c.speed = 50; f.adjustSpeedFromScroll(1, c); assert(near(c.speed, 50)); }
    { Freecam f; FreecamConfig c; f.begin(0,0,0,0,0); f.addLook(0, 1e6, c); assert(near(f.pitch(), 90)); }
    { Freecam f; FreecamConfig c; FreecamInput in; in.forward = true; f.tick(in, c); assert(near(f.z(), 0)); }

    { FreeLook l; FreeLookConfig c; c.arrowSpeed = 4; ArrowKeys k; k.right = true;
      l.begin(10, 0); auto d = l.tick(k, c);
      assert(near(l.cameraYaw(), 14)); assert(near(d.yaw, 0));
      c.mode = FreeLookMode::Camera; d = l.tick(k, c);
      assert(near(l.cameraYaw(), 14)); assert(near(d.yaw, 4)); }
    { FreeLook l; FreeLookConfig c; l.begin(0, 0);
      assert(!l.addCameraLook(8, 8, c));
      c.mode = FreeLookMode::Camera; assert(l.addCameraLook(16, 0, c)); assert(near(l.cameraYaw(), 2));
      l.addCameraLook(0, 1e6, c); assert(near(l.cameraPitch(), 90)); }
    { FreeLook l; FreeLookConfig c; c.arrowsControlOpposite = false; ArrowKeys k; k.left = true;
      l.begin(0, 0); auto d = l.tick(k, c); assert(near(d.yaw, 0)); assert(near(l.cameraYaw(), 0)); }

    { FullBright b; double v = b.enable(1.0); assert(near(v, 10.0));
      double out = 0; assert(!b.reassert(10.0, out)); assert(b.reassert(2.0, out)); assert(near(out, 10.0));
      assert(near(b.disable(), 1.0)); assert(!b.applied()); }
    { FullBright b; b.enable(10.0); assert(near(b.disable(), 1.0)); }

    std::puts("all core tests passed");
    return 0;
}
