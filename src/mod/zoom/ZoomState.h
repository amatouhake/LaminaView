#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>

namespace lamina_view::zoom {

/// Game-independent zoom math and hold state.
///
/// A zoom *level* of N means one Nth of the normal FOV; sensitivity scales as
/// 1/N so the same mouse movement turns the view by the same on-screen
/// amount.
///
/// Threading: the hold/level are written on the input thread (key and wheel
/// handlers) and read on the render thread (camera hook) and the game thread
/// (turn-delta hook), so all shared fields are atomic. All functions clamp
/// their inputs. Not copyable because of the atomics; configure in place.
class ZoomState {
public:
    ZoomState() = default;
    ZoomState(float defaultLevel, float minLevel, float maxLevel, float wheelStep) {
        configure(defaultLevel, minLevel, maxLevel, wheelStep);
    }
    ZoomState(ZoomState const&)            = delete;
    ZoomState& operator=(ZoomState const&) = delete;

    void configure(float defaultLevel, float minLevel, float maxLevel, float wheelStep) {
        float lo = std::min(minLevel, maxLevel);
        float hi = std::max(minLevel, maxLevel);
        mMinLevel.store(lo, std::memory_order_relaxed);
        mMaxLevel.store(hi, std::memory_order_relaxed);
        mDefaultLevel.store(sanitizeLevel(defaultLevel, lo, hi), std::memory_order_relaxed);
        float step = std::fabs(wheelStep);
        if (!(step > 0.0f)) step = 0.5f; // NaN or 0: harmless inert default.
        mWheelStep.store(step, std::memory_order_relaxed);
        mLevel.store(mDefaultLevel.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    void press() {
        mLevel.store(sanitizeLevel(mLevel.load(std::memory_order_relaxed), minLevel(), maxLevel()));
        mHeld.store(true, std::memory_order_relaxed);
    }

    void release() { mHeld.store(false, std::memory_order_relaxed); }

    /// World unload / disconnect / dimension change / focus loss: forget the
    /// hold so a stuck key can never leave the FOV narrowed.
    void resetTransient() {
        mHeld.store(false, std::memory_order_relaxed);
        mLevel.store(mDefaultLevel.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    /// Mouse-wheel notch while held: +1 zooms in, -1 zooms out. Ignored
    /// unless currently held.
    void wheel(int direction) {
        if (!held() || direction == 0) return;
        float step = mWheelStep.load(std::memory_order_relaxed);
        mLevel.store(clamp(mLevel.load(std::memory_order_relaxed) + (direction > 0 ? step : -step)));
    }

    [[nodiscard]] bool  held() const { return mHeld.load(std::memory_order_relaxed); }
    [[nodiscard]] float level() const { return mLevel.load(std::memory_order_relaxed); }
    [[nodiscard]] float minLevel() const { return mMinLevel.load(std::memory_order_relaxed); }
    [[nodiscard]] float maxLevel() const { return mMaxLevel.load(std::memory_order_relaxed); }

    /// FOV actually applied while zoomed: base / level. Never below 1 degree
    /// and never above the base FOV, whatever the caller passes.
    [[nodiscard]] float zoomedFov(float baseFov) const {
        if (!(baseFov > 0.0f)) return baseFov;
        return std::clamp(baseFov / level(), 1.0f, baseFov);
    }

    /// Sensitivity multiplier at the current level: 1/level. Guards against
    /// degenerate levels so sensitivity can never flip sign or explode.
    [[nodiscard]] float sensitivityScale() const {
        float current = level();
        if (!(current > 0.0f)) return 1.0f;
        return 1.0f / current;
    }

    [[nodiscard]] float clamp(float level) const { return std::clamp(level, minLevel(), maxLevel()); }

private:
    /// Out-of-range values clamp to the nearest bound; NaN ( unordered )
    /// falls back to the in-range default 3x.
    static float sanitizeLevel(float level, float lo, float hi) {
        if (level != level) return std::clamp(3.0f, lo, hi);
        return std::clamp(level, lo, hi);
    }

    std::atomic<float> mDefaultLevel{3.0f};
    std::atomic<float> mMinLevel{1.5f};
    std::atomic<float> mMaxLevel{10.0f};
    std::atomic<float> mWheelStep{0.5f};
    std::atomic<float> mLevel{3.0f};
    std::atomic<bool>  mHeld{false};
};

} // namespace lamina_view::zoom
