#include "mod/zoom/Zoom.h"

#include "mod/Config.h"
#include "mod/LaminaView.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/client/ClientExitLevelEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "ll/api/input/KeyRegistry.h"
#include "ll/api/memory/Hook.h"

#include "mc/client/game/IClientInstance.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/client/renderer/game/LevelRendererPlayer.h"
#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/input/MouseAction.h"
#include "mc/deps/renderer/Camera.h"

#include <string>

class Player;

namespace lamina_view::zoom {

namespace {

constexpr std::string_view kKeyName = "zoom";

LL_TYPE_INSTANCE_HOOK(
    SetupCameraHook,
    ll::memory::HookPriority::Normal,
    LevelRendererPlayer,
    &LevelRendererPlayer::setupCamera,
    void,
    ::mce::Camera& camera,
    float          a
) {
    origin(camera, a);
    auto& zoom = Zoom::getInstance();
    // Pass-through unless zoomed: the vanilla FOV (including spyglass and
    // status-effect modifiers) is never otherwise touched.
    if (!zoom.installed() || !zoom.held()) return;
    camera.mFov = zoom.zoomedFov(camera.mFov);
}

LL_TYPE_INSTANCE_HOOK(
    ApplyTurnDeltaHook,
    ll::memory::HookPriority::Normal,
    LocalPlayer,
    &LocalPlayer::_applyTurnDelta,
    void,
    ::Vec2 const& turnOffset
) {
    auto& zoom = Zoom::getInstance();
    if (!zoom.installed() || !zoom.held()) {
        origin(turnOffset);
        return;
    }
    // Scale the look delta so the same mouse movement covers the same
    // on-screen angle at any zoom level. Spyglass scoping applies its own
    // damping separately; zoom never disables it.
    ::Vec2 scaled{turnOffset.x * zoom.sensitivityScale(), turnOffset.z * zoom.sensitivityScale()};
    origin(scaled);
}

// Dimension swaps rebuild the render state; a hold spanning one must never
// survive it.
LL_TYPE_INSTANCE_HOOK(
    DimensionChangedHook,
    ll::memory::HookPriority::Normal,
    LevelRendererPlayer,
    &LevelRendererPlayer::$onWillChangeDimension,
    void,
    ::Player& player
) {
    Zoom::getInstance().onWorldLeft();
    origin(player);
}

using Hooks = ll::memory::HookRegistrar<SetupCameraHook, ApplyTurnDeltaHook, DimensionChangedHook>;

// Only the HUD screen leaves hotkeys and the wheel to gameplay; everywhere
// else (inventory, chest, pause, chat, ...) input belongs to the UI.
bool isHudScreen(std::string const& screenName) { return screenName.rfind("hud_screen", 0) == 0; }

} // namespace

Zoom& Zoom::getInstance() {
    static Zoom instance;
    return instance;
}

void Zoom::load(Config const& config) noexcept {
    mOwnedConfig = std::make_unique<Config>(config);
    if (!config.zoom.enabled) return;
    mState = ZoomState(config.zoom.defaultLevel, config.zoom.minLevel, config.zoom.maxLevel, config.zoom.wheelStep);
    try {
        auto& key = ll::input::KeyRegistry::getInstance().getOrCreateKey(kKeyName, {config.zoom.keyCode});
        key.registerButtonDownHandler([this](::FocusImpact, ::IClientInstance& client) { onPressed(client); });
        key.registerButtonUpHandler([this](::FocusImpact, ::IClientInstance&) { onReleased(); });
        mLoaded = true;
    } catch (...) {
        mLoaded = false;
    }
}

void Zoom::install() noexcept {
    if (mInstalled || !mLoaded) return;
    try {
        Hooks::hook();
        mWheelListener =
            ll::event::EventBus::getInstance().emplaceListener<ll::event::input::MouseInputEvent>(
                [this](ll::event::input::MouseInputEvent& event) {
                    // Only wheel events are ours; movement and buttons pass
                    // through untouched so inventory/menu scrolling and
                    // clicking keep working. The hold itself is HUD-gated in
                    // onPressed, so an unconsumed wheel here means zoom is
                    // not active.
                    if (event.actionButtonId() != ::MouseAction::ActionWheel) return;
                    if (!installed() || !held()) return;
                    if (event.buttonData() != ::MouseAction::DataUp
                        && event.buttonData() != ::MouseAction::DataDown) {
                        return;
                    }
                    onWheel(event.buttonData() == ::MouseAction::DataDown ? 1 : -1);
                    event.cancel();
                }
            );
        mExitListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::ClientExitLevelEvent>(
            [this](ll::event::ClientExitLevelEvent&) { onWorldLeft(); }
        );
        mInstalled = true;
    } catch (...) {
        mInstalled = false;
    }
}

void Zoom::uninstall() noexcept {
    if (!mInstalled) return;
    try {
        if (mWheelListener) {
            ll::event::EventBus::getInstance().removeListener(mWheelListener);
            mWheelListener.reset();
        }
        if (mExitListener) {
            ll::event::EventBus::getInstance().removeListener(mExitListener);
            mExitListener.reset();
        }
        Hooks::unhook();
    } catch (...) {
    }
    // Hooks are gone so FOV/sensitivity are already vanilla again; drop the
    // transient hold so a later install starts clean.
    mState.resetTransient();
    mInstalled = false;
}

void Zoom::onPressed(IClientInstance& client) {
    if (!mInstalled || held()) return;
    // A menu/container screen open under the pointer owns the key, not zoom.
    if (!isHudScreen(client.getScreenName())) return;
    mState.press();
    LaminaView::getInstance().getSelf().getLogger().debug("Zoom on (level {})", mState.level());
}

void Zoom::onReleased() {
    if (!mInstalled || !held()) return;
    mState.release();
    LaminaView::getInstance().getSelf().getLogger().debug("Zoom off");
}

void Zoom::onWheel(int direction) { mState.wheel(direction); }

void Zoom::onWorldLeft() {
    if (!mInstalled) return;
    if (held()) LaminaView::getInstance().getSelf().getLogger().debug("Zoom released (world left)");
    mState.resetTransient();
}

void Zoom::onFocusLost() {
    if (!mInstalled) return;
    if (held()) LaminaView::getInstance().getSelf().getLogger().debug("Zoom released (focus lost)");
    mState.resetTransient();
}

} // namespace lamina_view::zoom
