#pragma once

namespace lamina_view::vision {

/// Game-independent fullbright state. The game-facing hook layer only calls
/// setEnabled(); the render hook reads enabled(). Idempotent by construction:
/// enabling twice or disabling twice changes nothing observable.
class NightVisionState {
public:
    void setEnabled(bool enabled) { mEnabled = enabled; }
    void toggle() { mEnabled = !mEnabled; }

    [[nodiscard]] bool enabled() const { return mEnabled; }

private:
    bool mEnabled{false};
};

} // namespace lamina_view::vision
