#pragma once

#include "ll/api/event/ListenerBase.h"

#include "mod/vision/NightVisionState.h"

#include <memory>

class BaseLightTextureImageBuilder;
class BaseLightData;
class IClientInstance;
namespace mce {
struct Image;
}

namespace lamina_view {
struct Config;
}

namespace lamina_view::vision {

/// Pure client-side fullbright-style NightVision.
///
/// Implementation: hooks `BaseLightTextureImageBuilder::buildImage` (the
/// virtual every dimension's light-texture builder shares) and forces the
/// light data the image is built from to night-vision levels before calling
/// through. Nothing is written to the vanilla brightness option, no
/// status effect is injected, no packet is sent. Underwater keeps its own
/// path but is brightened to the same night-vision scale, matching the
/// vanilla night-vision look. Disabling restores rendering exactly because
/// the hook becomes a pass-through.
///
/// Separable from Zoom: own state, key, config section, hooks and cleanup.
class NightVision {
public:
    static NightVision& getInstance();

    /// Registers the toggle key (game keyboard settings, remappable as
    /// "LaminaView.nightvision"). Safe to call when disabled: does nothing.
    /// Never throws: one failing feature must not break the other.
    void load(Config const& config) noexcept;

    /// Installs hooks and level-exit/leave cleanup. Safe when disabled.
    void install() noexcept;
    /// Removes hooks and clears state (rendering already restored because
    /// the hook is gone). Idempotent.
    void uninstall() noexcept;

    [[nodiscard]] bool installed() const { return mInstalled; }
    [[nodiscard]] bool enabled() const { return mState.enabled(); }

    /// Toggle entry point for the key handler.
    void toggle();
    /// World unload / disconnect / dimension change: NightVision is a
    /// render-layer override, so there is nothing world-bound to clear; the
    /// toggle intentionally survives them. Present for symmetry with Zoom
    /// and future-proofing.
    void onWorldLeft() {}

public:
    static void applyOverride(BaseLightData& lightData);

private:

    NightVisionState        mState;
    bool                    mLoaded{false};
    bool                    mInstalled{false};
    ll::event::ListenerPtr  mExitListener;
    std::unique_ptr<Config> mOwnedConfig;
};
} // namespace lamina_view::vision
