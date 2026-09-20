#pragma once

#include "ll/api/event/ListenerBase.h"

#include "mod/zoom/ZoomState.h"

#include <memory>

class IClientInstance;

namespace lamina_view {
struct Config;
}

namespace lamina_view::zoom {

/// Hold-to-zoom. FOV-based only: while the key is held the camera setup sees
/// a narrowed FOV; on release the exact previous FOV returns because the hook
/// becomes a pass-through again. Mouse-look sensitivity is scaled by 1/level
/// by scaling the turn delta while held and restored on release.
/// The mouse wheel adjusts the level only while zoomed; when no zoom is
/// active the wheel is never consumed, and vanilla spyglass scoping is left
/// untouched.
///
/// Separable from NightVision: own state, keys, config section, hooks and
/// cleanup.
class Zoom {
public:
    static Zoom& getInstance();

    /// Registers the hold key (down/up handlers) and the wheel listener.
    /// Safe to call when disabled: does nothing. Never throws.
    void load(Config const& config) noexcept;

    /// Installs hooks and level-exit cleanup. Safe when disabled.
    void install() noexcept;
    /// Removes hooks/listeners and clears transient hold state. Idempotent.
    void uninstall() noexcept;

    [[nodiscard]] bool installed() const { return mInstalled; }
    [[nodiscard]] bool held() const { return mState.held(); }
    [[nodiscard]] float level() const { return mState.level(); }
    [[nodiscard]] float zoomedFov(float base) const { return mState.zoomedFov(base); }
    [[nodiscard]] float sensitivityScale() const { return mState.sensitivityScale(); }

    void onPressed(IClientInstance& client);
    void onReleased();
    /// Mouse-wheel notch: +1 in, -1 out. Only acts while held.
    void onWheel(int direction);

    /// World unload / disconnect / dimension change: a stuck hold can never
    /// leave the FOV narrowed or sensitivity scaled.
    void onWorldLeft();
    /// Focus loss: same guarantee. NOTE: not yet wired to an event — the SDK
    /// exposes no focus-loss event; wire when one is available.
    void onFocusLost();

private:
    ZoomState                   mState;
    bool                        mLoaded{false};
    bool                        mInstalled{false};
    ll::event::ListenerPtr      mExitListener;
    ll::event::ListenerPtr      mWheelListener;
    std::unique_ptr<Config>     mOwnedConfig;
};

} // namespace lamina_view::zoom
