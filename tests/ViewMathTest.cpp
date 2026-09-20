// Focused checks for the pure zoom math/state and NightVision toggle state.
// Built with `xmake build LaminaViewTests`, run with `xmake run LaminaViewTests`.

#include "mod/vision/NightVisionState.h"
#include "mod/zoom/ZoomState.h"

#include <cmath>
#include <cstdio>

using lamina_view::vision::NightVisionState;
using lamina_view::zoom::ZoomState;

namespace {

int gFailures = 0;

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

void check(bool ok, char const* what, int line) {
    if (!ok) {
        ++gFailures;
        std::printf("FAIL line %d: %s\n", line, what);
    }
}

#define CHECK(expr) check((expr), #expr, __LINE__)

void testClampBounds() {
    ZoomState z(3.0f, 1.5f, 10.0f, 0.5f);
    CHECK(near(z.clamp(1.0f), 1.5f));
    CHECK(near(z.clamp(99.0f), 10.0f));
    CHECK(near(z.clamp(4.0f), 4.0f));
    CHECK(near(z.clamp(1.5f), 1.5f));
    CHECK(near(z.clamp(10.0f), 10.0f));
}

void testWheelStaysInBounds() {
    ZoomState z(3.0f, 1.5f, 10.0f, 0.5f);
    z.press();
    for (int i = 0; i < 100; ++i) z.wheel(1);
    CHECK(near(z.level(), 10.0f));
    for (int i = 0; i < 100; ++i) z.wheel(-1);
    CHECK(near(z.level(), 1.5f));
}

void testWheelIgnoredWhenNotHeld() {
    ZoomState z(3.0f, 1.5f, 10.0f, 0.5f);
    z.wheel(1);
    CHECK(near(z.level(), 3.0f));
    z.wheel(-1);
    CHECK(near(z.level(), 3.0f));
    z.wheel(0);
    CHECK(near(z.level(), 3.0f));
}

void testWheelStep() {
    ZoomState z(3.0f, 1.5f, 10.0f, 0.5f);
    z.press();
    z.wheel(1);
    CHECK(near(z.level(), 3.5f));
    z.wheel(-1);
    z.wheel(-1);
    CHECK(near(z.level(), 2.5f));
}

void testZoomedFov() {
    ZoomState z(4.0f, 1.5f, 10.0f, 0.5f);
    z.press();
    CHECK(near(z.zoomedFov(90.0f), 22.5f));
    // Never below 1 degree, never above the base FOV.
    ZoomState deep(10.0f, 1.5f, 100.0f, 1.0f);
    deep.press();
    CHECK(deep.zoomedFov(90.0f) >= 1.0f);
    ZoomState shallow(1.5f, 1.0f, 10.0f, 0.5f);
    shallow.press();
    CHECK(shallow.zoomedFov(90.0f) <= 90.0f);
    // Degenerate base FOV passes through untouched.
    CHECK(near(z.zoomedFov(0.0f), 0.0f));
}

void testSensitivityScale() {
    ZoomState z(4.0f, 1.5f, 10.0f, 0.5f);
    z.press();
    CHECK(near(z.sensitivityScale(), 0.25f));
    // FOV division and sensitivity scaling are exact inverses: the same
    // mouse movement covers the same on-screen angle at any level.
    CHECK(near(z.zoomedFov(90.0f) / 90.0f, z.sensitivityScale()));
}

void testReleaseRestoresExactly() {
    ZoomState z(3.0f, 1.5f, 10.0f, 0.5f);
    z.press();
    z.wheel(1);
    z.wheel(1);
    CHECK(z.held());
    float const heldLevel = z.level();
    CHECK(heldLevel > 3.0f);
    z.release();
    CHECK(!z.held());
    // Re-pressing resumes from the adjusted level: nothing is lost, and a
    // fresh reset returns to the configured default.
    z.press();
    CHECK(near(z.level(), heldLevel));
    z.resetTransient();
    CHECK(!z.held());
    CHECK(near(z.level(), 3.0f));
}

void testResetTransientClearsStuckHold() {
    ZoomState z(3.0f, 1.5f, 10.0f, 0.5f);
    z.press();
    z.wheel(1);
    z.resetTransient();
    CHECK(!z.held());
    CHECK(near(z.level(), 3.0f));
}

void testSwappedBoundsSanitized() {
    ZoomState z(3.0f, 10.0f, 1.5f, 0.5f);
    CHECK(near(z.minLevel(), 1.5f));
    CHECK(near(z.maxLevel(), 10.0f));
    CHECK(near(z.level(), 3.0f));
}

void testOutOfRangeDefaultClampsToBound() {
    ZoomState hi(99.0f, 1.5f, 10.0f, 0.5f);
    CHECK(near(hi.level(), 10.0f));
    ZoomState lo(0.1f, 1.5f, 10.0f, 0.5f);
    CHECK(near(lo.level(), 1.5f));
}

void testNightVisionToggle() {
    NightVisionState nv;
    CHECK(!nv.enabled());
    nv.toggle();
    CHECK(nv.enabled());
    nv.toggle();
    CHECK(!nv.enabled());
}

void testNightVisionIdempotent() {
    NightVisionState nv;
    nv.setEnabled(true);
    nv.setEnabled(true);
    CHECK(nv.enabled());
    nv.setEnabled(false);
    nv.setEnabled(false);
    CHECK(!nv.enabled());
}

} // namespace

int main() {
    testClampBounds();
    testWheelStaysInBounds();
    testWheelIgnoredWhenNotHeld();
    testWheelStep();
    testZoomedFov();
    testSensitivityScale();
    testReleaseRestoresExactly();
    testResetTransientClearsStuckHold();
    testSwappedBoundsSanitized();
    testOutOfRangeDefaultClampsToBound();
    testNightVisionToggle();
    testNightVisionIdempotent();

    if (gFailures == 0) {
        std::printf("ViewMath tests: all passed\n");
        return 0;
    }
    std::printf("ViewMath tests: %d failure(s)\n", gFailures);
    return 1;
}
