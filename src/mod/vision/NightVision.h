#pragma once

#include "mod/vision/NightVisionState.h"

#include <atomic>

class BaseLightData;
class IClientInstance;

namespace lamina_view {
struct Config;
}

namespace lamina_view::vision {

/// Pure client-side fullbright-style NightVision.
///
/// Implementation: hooks `BaseLightTextureImageBuilder::createBaseLightTextureData`
/// (shared by the Overworld and The End) and the Nether builder's override,
/// and forces the per-frame light data they produce to night-vision levels.
/// The game only re-rasterizes its light LUT when that data changes, so this
/// is the one place the override is visible (a `buildImage` hook never runs
/// while nothing else changes - verified in-game). Nothing is written to the
/// vanilla brightness option, no status effect is injected, no packet is
/// sent. Underwater keeps its own path: only a pre-existing underwater scale
/// is raised, the underwater flag itself is never set, so the water tint is
/// unchanged above water. Disabling restores rendering exactly because the
/// produced data goes back to vanilla and the game rebuilds the LUT from it.
///
/// The toggle key is HUD-gated like Zoom: presses from any non-HUD screen
/// (chat, Creative search, anvil, inventory, ...) belong to the UI and are
/// ignored, so typing never toggles. The toggle intentionally survives world
/// unload/disconnect/dimension changes: it is a render-layer override with
/// nothing world-bound, so there is no state that could stick.
///
/// Threading: the toggle is written on the input thread (key handler) and
/// read on the render thread (light-texture hook); all cross-thread fields
/// are atomic.
///
/// Separable from Zoom: own state, key, config section, hooks and cleanup.
class NightVision {
public:
    static NightVision& getInstance();

    /// Registers the toggle key (game keyboard settings, remappable as
    /// "LaminaView.nightvision"). Safe to call when disabled: does nothing.
    /// Never throws: one failing feature must not break the other.
    void load(Config const& config) noexcept;

    /// Installs the hook. Safe when disabled.
    void install() noexcept;
    /// Removes the hook and clears state (rendering already restored because
    /// the hook is gone). Idempotent.
    void uninstall() noexcept;

    [[nodiscard]] bool installed() const { return mInstalled.load(std::memory_order_relaxed); }
    [[nodiscard]] bool enabled() const { return mState.enabled(); }

    /// Toggle entry point for the key handler. HUD-gated: non-HUD screens
    /// own the key, not NightVision.
    void toggle(IClientInstance& client);

    /// Render-thread entry point from the light-data hooks: applies the
    /// override to freshly produced data when enabled, otherwise leaves it.
    void overrideProducedData(BaseLightData* lightData);

    static void applyOverride(BaseLightData& lightData);

private:
    NightVisionState  mState;
    std::atomic<bool> mLoaded{false};
    std::atomic<bool> mInstalled{false};
#ifdef LAMINAVIEW_TRACE
    std::atomic<bool> mTraceApplied{false};
#endif
};
} // namespace lamina_view::vision
