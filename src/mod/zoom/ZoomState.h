#pragma once

#include <algorithm>
#include <cmath>

namespace lamina_view::zoom {

/// Game-independent zoom math and hold state.
///
/// A zoom *level* of N means one Nth of the normal FOV; sensitivity scales as
/// 1/N so the same mouse movement turns the view by the same on-screen
/// amount. All functions are pure and clamp their inputs.
class ZoomState {
public:
    ZoomState() = default;
    ZoomState(float defaultLevel, float minLevel, float maxLevel, float wheelStep)
    : mDefaultLevel(sanitizeLevel(defaultLevel, minLevel, maxLevel)),
      mMinLevel(std::min(minLevel, maxLevel)),
      mMaxLevel(std::max(minLevel, maxLevel)),
      mWheelStep(std::fabs(wheelStep)),
      mLevel(mDefaultLevel) {}

    void press() {
        mHeld = true;
        mLevel = sanitizeLevel(mLevel, mMinLevel, mMaxLevel);
    }

    void release() { mHeld = false; }

    /// World unload / disconnect / dimension change / focus loss: forget the
    /// hold so a stuck key can never leave the FOV narrowed.
    void resetTransient() {
        mHeld  = false;
        mLevel = mDefaultLevel;
    }

    /// Mouse-wheel notch while held: +1 zooms in, -1 zooms out. Ignored
    /// unless currently held.
    void wheel(int direction) {
        if (!mHeld || direction == 0) return;
        mLevel = clamp(mLevel + (direction > 0 ? mWheelStep : -mWheelStep));
    }

    [[nodiscard]] bool held() const { return mHeld; }
    [[nodiscard]] float level() const { return mLevel; }
    [[nodiscard]] float minLevel() const { return mMinLevel; }
    [[nodiscard]] float maxLevel() const { return mMaxLevel; }

    /// FOV actually applied while zoomed: base / level. Never below 1 degree
    /// and never above the base FOV, whatever the caller passes.
    [[nodiscard]] float zoomedFov(float baseFov) const {
        if (!(baseFov > 0.0f)) return baseFov;
        return std::clamp(baseFov / mLevel, 1.0f, baseFov);
    }

    /// Sensitivity multiplier at the current level: 1/level. Guards against
    /// degenerate levels so sensitivity can never flip sign or explode.
    [[nodiscard]] float sensitivityScale() const {
        if (!(mLevel > 0.0f)) return 1.0f;
        return 1.0f / mLevel;
    }

    [[nodiscard]] float clamp(float level) const { return std::clamp(level, mMinLevel, mMaxLevel); }

private:
    static float sanitizeLevel(float level, float minLevel, float maxLevel) {
        float lo = std::min(minLevel, maxLevel);
        float hi = std::max(minLevel, maxLevel);
        if (!(level >= lo && level <= hi)) return std::clamp(3.0f, lo, hi);
        return level;
    }

    float mDefaultLevel{3.0f};
    float mMinLevel{1.5f};
    float mMaxLevel{10.0f};
    float mWheelStep{0.5f};
    float mLevel{3.0f};
    bool  mHeld{false};
};

} // namespace lamina_view::zoom
