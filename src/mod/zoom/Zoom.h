#pragma once

#include "ll/api/event/ListenerBase.h"

#include "mod/zoom/ZoomState.h"

#include <atomic>
#include <string>

class IClientInstance;

namespace lamina_view {
struct Config;
}

namespace lamina_view::zoom {

/// Hold-to-zoom. FOV-based only: while the key is held the camera setup sees
/// a narrowed FOV; on release the exact previous FOV returns because the hook
/// becomes a pass-through again. Mouse-look sensitivity is scaled by 1/level
/// by scaling the turn delta while held and restored on release.
/// The mouse wheel adjusts the level only while zoomed from the HUD screen;
/// anywhere else the wheel is never consumed, and vanilla spyglass scoping
/// is left untouched.
///
/// A stuck hold is impossible by construction: the hold clears on key-up, on
/// any non-HUD screen (menu/inventory/pause/chat opened mid-hold, watched
/// every rendered frame), on app focus loss / suspend, and on world
/// unload/disconnect/dimension change.
///
/// Threading: key/wheel handlers run on the input thread, the camera and
/// turn-delta hooks on render/game threads. All cross-thread fields
/// (installed/loaded flags, client pointer, hold/level in ZoomState) are
/// atomic.
///
/// Separable from NightVision: own state, keys, config section, hooks and
/// cleanup.
class Zoom {
public:
    static Zoom& getInstance();

    /// Registers the hold key (down/up handlers) and the wheel listener.
    /// Safe to call when disabled: does nothing. Never throws. Zoom bounds
    /// are validated here: non-positive bounds fall back to the 1.5x-10x
    /// default range.
    void load(Config const& config) noexcept;

    /// Installs hooks and level-exit cleanup. Safe when disabled.
    void install() noexcept;
    /// Removes hooks/listeners and clears transient hold state. Idempotent.
    void uninstall() noexcept;

    [[nodiscard]] bool installed() const { return mInstalled.load(std::memory_order_relaxed); }
    [[nodiscard]] bool held() const { return mState.held(); }
    [[nodiscard]] float level() const { return mState.level(); }
    [[nodiscard]] float zoomedFov(float base) const { return mState.zoomedFov(base); }
    [[nodiscard]] float sensitivityScale() const { return mState.sensitivityScale(); }

    void onPressed(IClientInstance& client);
    void onReleased();
    /// Mouse-wheel notch: +1 in, -1 out. Only acts while held from the HUD.
    void onWheel(int direction, IClientInstance* client);

    /// World unload / disconnect / dimension change: a stuck hold can never
    /// leave the FOV narrowed or sensitivity scaled.
    void onWorldLeft();
    /// App focus loss / suspend: same guarantee.
    void onFocusLost();

private:
    /// The client that pressed the key (the long-lived game client,
    /// cleared on uninstall). Used to re-check the current screen on the
    /// wheel path and in the per-frame screen watcher.
    std::atomic<IClientInstance*> mClient{nullptr};

    ZoomState                   mState;
    std::atomic<bool>           mLoaded{false};
    std::atomic<bool>           mInstalled{false};
    ll::event::ListenerPtr      mExitListener;
    ll::event::ListenerPtr      mWheelListener;
    ll::event::ListenerPtr      mScreenListener;
};

} // namespace lamina_view::zoom
