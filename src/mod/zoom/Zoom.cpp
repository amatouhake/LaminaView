#include "mod/zoom/Zoom.h"

#include "mod/Config.h"
#include "mod/LaminaView.h"
#include "mod/Screens.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/client/ClientExitLevelEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "ll/api/event/render/UIRenderEvent.h"
#include "ll/api/input/KeyRegistry.h"
#include "ll/api/memory/Hook.h"

#include "mc/client/game/IClientInstance.h"
#include "mc/client/game/MinecraftGame.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/client/renderer/game/LevelRendererPlayer.h"
#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/input/MouseAction.h"
#include "mc/deps/renderer/Camera.h"

#include <string>
#include <type_traits>

class Player;

namespace lamina_view::zoom {

namespace {

constexpr std::string_view kKeyName = "zoom";

// Vec2's second axis is `z` (x, y/z/b/p union) in this SDK generation: yaw
// lives in x, pitch in z. Enforced at compile time so an SDK regeneration
// that renames the axis breaks the build instead of silently scrambling
// look direction.
static_assert(std::is_same_v<decltype(::Vec2::x), float>);
static_assert(std::is_same_v<decltype(::Vec2::z), float>);

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

// Alt-tab / Win-key / app suspend: the key-up may never arrive. MinecraftGame
// owns the focus-loss virtual (ClientInstance has only onAppSuspended).
LL_TYPE_INSTANCE_HOOK(
    AppFocusLostHook,
    ll::memory::HookPriority::Normal,
    MinecraftGame,
    &MinecraftGame::$onAppFocusLost,
    void
) {
    Zoom::getInstance().onFocusLost();
    origin();
}

using Hooks =
    ll::memory::HookRegistrar<SetupCameraHook, ApplyTurnDeltaHook, DimensionChangedHook, AppFocusLostHook>;

} // namespace

Zoom& Zoom::getInstance() {
    static Zoom instance;
    return instance;
}

void Zoom::load(Config const& config) noexcept {
    if (!config.zoom.enabled) return;
    // Bounds must be positive zoom factors; anything else falls back to the
    // 1.5x-10x, 3x-default, 0.5-step schema so a bad config can never invert
    // the FOV or divide by zero.
    float minLevel = config.zoom.minLevel > 0.0f ? config.zoom.minLevel : 1.5f;
    float maxLevel = config.zoom.maxLevel > 0.0f ? config.zoom.maxLevel : 10.0f;
    if (minLevel > maxLevel) std::swap(minLevel, maxLevel);
    mState.configure(config.zoom.defaultLevel, minLevel, maxLevel, config.zoom.wheelStep);
    try {
        auto& key = ll::input::KeyRegistry::getInstance().getOrCreateKey(kKeyName, {config.zoom.keyCode});
        key.registerButtonDownHandler([this](::FocusImpact, ::IClientInstance& client) { onPressed(client); });
        key.registerButtonUpHandler([this](::FocusImpact, ::IClientInstance&) { onReleased(); });
        mLoaded.store(true, std::memory_order_relaxed);
    } catch (...) {
        mLoaded.store(false, std::memory_order_relaxed);
    }
}

void Zoom::install() noexcept {
    if (installed() || !mLoaded.load(std::memory_order_relaxed)) return;
    try {
        Hooks::hook();
        mWheelListener =
            ll::event::EventBus::getInstance().emplaceListener<ll::event::input::MouseInputEvent>(
                [this](ll::event::input::MouseInputEvent& event) {
                    // Only wheel events are ours; movement and buttons pass
                    // through untouched so inventory/menu scrolling and
                    // clicking keep working. HUD-gated per event against the
                    // pressing client: holding C on the HUD, opening a menu
                    // mid-hold and scrolling must scroll the menu, not the
                    // zoom level — the hold itself is dropped by the screen
                    // watcher before the next frame anyway.
                    if (event.actionButtonId() != ::MouseAction::ActionWheel) return;
                    if (!installed() || !held()) return;
                    if (event.buttonData() != ::MouseAction::DataUp
                        && event.buttonData() != ::MouseAction::DataDown) {
                        return;
                    }
                    auto* client = mClient.load(std::memory_order_relaxed);
                    if (!client || !lamina_view::isHudScreen(client->getScreenName())) return;
                    // Universal convention: scroll up zooms in, scroll down
                    // zooms out.
                    onWheel(event.buttonData() == ::MouseAction::DataUp ? 1 : -1, client);
                    event.cancel();
                }
            );
        // Any non-HUD screen opened mid-hold (inventory, pause, chat, ...)
        // owns the input from that frame on: drop the hold so neither FOV
        // nor the wheel can stick. Runs every frame while installed.
        mScreenListener =
            ll::event::EventBus::getInstance().emplaceListener<ll::event::AfterUIRenderEvent>(
                [this](ll::event::AfterUIRenderEvent&) {
                    if (!installed() || !held()) return;
                    auto* client = mClient.load(std::memory_order_relaxed);
                    if (!client) return;
                    if (!lamina_view::isHudScreen(client->getScreenName())) onReleased();
                }
            );
        mExitListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::ClientExitLevelEvent>(
            [this](ll::event::ClientExitLevelEvent&) { onWorldLeft(); }
        );
        mInstalled.store(true, std::memory_order_relaxed);
    } catch (...) {
        mInstalled.store(false, std::memory_order_relaxed);
    }
}

void Zoom::uninstall() noexcept {
    if (!installed()) return;
    mInstalled.store(false, std::memory_order_relaxed);
    try {
        if (mWheelListener) {
            ll::event::EventBus::getInstance().removeListener(mWheelListener);
            mWheelListener.reset();
        }
        if (mScreenListener) {
            ll::event::EventBus::getInstance().removeListener(mScreenListener);
            mScreenListener.reset();
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
    mClient.store(nullptr, std::memory_order_relaxed);
    mState.resetTransient();
}

void Zoom::onPressed(IClientInstance& client) {
    if (!installed() || held()) return;
    // A menu/container screen open under the pointer owns the key, not zoom.
    std::string const screen = client.getScreenName();
#ifdef LAMINAVIEW_TRACE
    LaminaView::getInstance().getSelf().getLogger().debug("Zoom press on screen '{}'", screen);
#endif
    if (!lamina_view::isHudScreen(screen)) return;
    mClient.store(&client, std::memory_order_relaxed);
    mState.press();
    LaminaView::getInstance().getSelf().getLogger().debug("Zoom on (level {})", mState.level());
}

void Zoom::onReleased() {
    if (!installed() || !held()) return;
    mState.release();
    LaminaView::getInstance().getSelf().getLogger().debug("Zoom off");
}

void Zoom::onWheel(int direction, IClientInstance* client) {
    if (!installed() || !held()) return;
    // The pressing client must still show the HUD; otherwise the wheel
    // belongs to the menu that opened mid-hold.
    auto* current = mClient.load(std::memory_order_relaxed);
    if (client != nullptr && client != current) return;
    if (current == nullptr || !lamina_view::isHudScreen(current->getScreenName())) return;
    mState.wheel(direction);
}

void Zoom::onWorldLeft() {
    if (!installed()) return;
    if (held()) LaminaView::getInstance().getSelf().getLogger().debug("Zoom released (world left)");
    mState.resetTransient();
}

void Zoom::onFocusLost() {
    if (!installed()) return;
    if (held()) LaminaView::getInstance().getSelf().getLogger().debug("Zoom released (focus lost)");
    mState.resetTransient();
}

} // namespace lamina_view::zoom
