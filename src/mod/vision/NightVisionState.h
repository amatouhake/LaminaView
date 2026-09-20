#pragma once

#include <atomic>

namespace lamina_view::vision {

/// Game-independent fullbright state. The toggle is written on the input
/// thread (key handler) and read on the render thread (light-texture hook),
/// so the flag is atomic. Idempotent by construction: enabling twice or
/// disabling twice changes nothing observable.
class NightVisionState {
public:
    void setEnabled(bool enabled) { mEnabled.store(enabled, std::memory_order_relaxed); }
    void toggle() {
        bool expected = mEnabled.load(std::memory_order_relaxed);
        while (!mEnabled.compare_exchange_weak(expected, !expected, std::memory_order_relaxed)) {
        }
    }

    [[nodiscard]] bool enabled() const { return mEnabled.load(std::memory_order_relaxed); }

private:
    std::atomic<bool> mEnabled{false};
};

} // namespace lamina_view::vision
