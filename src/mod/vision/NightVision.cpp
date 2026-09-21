#include "mod/vision/NightVision.h"

#include "mod/Config.h"
#include "mod/LaminaView.h"
#include "mod/Screens.h"

#include "ll/api/input/KeyRegistry.h"
#include "ll/api/memory/Hook.h"

#include "mc/client/game/IClientInstance.h"
#include "mc/client/renderer/ptexture/BaseLightData.h"
#include "mc/client/renderer/ptexture/BaseLightTextureImageBuilder.h"
#include "mc/client/world/level/dimension/NetherLightTextureImageBuilder.h"

namespace lamina_view::vision {

namespace {

constexpr std::string_view kKeyName = "nightvision";

// The light texture is only re-rasterized when the per-frame BaseLightData
// differs from the previous one (LightTexture::refresh compares them;
// observed in-game: buildImage runs once on world entry and then never
// again while nothing changes). Overriding inside buildImage therefore does
// nothing visible. The override is applied where the data is produced:
// createBaseLightTextureData. The shared base implementation serves the
// Overworld and The End; the Nether builder overrides it, so that one is
// hooked as well. Toggling changes the produced data, the game notices the
// difference and rebuilds the LUT itself, on and off.
LL_TYPE_INSTANCE_HOOK(
    CreateBaseLightDataHook,
    ll::memory::HookPriority::Normal,
    BaseLightTextureImageBuilder,
    &BaseLightTextureImageBuilder::$createBaseLightTextureData,
    ::std::unique_ptr<::BaseLightData>,
    ::IClientInstance*     client,
    ::BaseLightData const& currentData
) {
    auto data = origin(client, currentData);
    NightVision::getInstance().overrideProducedData(data.get());
    return data;
}

LL_TYPE_INSTANCE_HOOK(
    CreateNetherLightDataHook,
    ll::memory::HookPriority::Normal,
    NetherLightTextureImageBuilder,
    &NetherLightTextureImageBuilder::$createBaseLightTextureData,
    ::std::unique_ptr<::BaseLightData>,
    ::IClientInstance*     client,
    ::BaseLightData const& currentData
) {
    auto data = origin(client, currentData);
    NightVision::getInstance().overrideProducedData(data.get());
    return data;
}

using Hooks = ll::memory::HookRegistrar<CreateBaseLightDataHook, CreateNetherLightDataHook>;

} // namespace

NightVision& NightVision::getInstance() {
    static NightVision instance;
    return instance;
}

void NightVision::load(Config const& config) noexcept {
    if (!config.nightvision.enabled) return;
    try {
        auto& key =
            ll::input::KeyRegistry::getInstance().getOrCreateKey(kKeyName, {config.nightvision.keyCode});
        // Post-remap gate, mirroring Zoom::onPressed: the key stays remappable
        // in game settings, but presses from any non-HUD screen (chat,
        // Creative search, anvil, inventory, ...) belong to the UI, not us.
        // Without this the toggle fires while typing (KeyRegistry binds every
        // input stack when no mapping stack is configured).
        key.registerButtonDownHandler([this](::FocusImpact, ::IClientInstance& client) { toggle(client); });
        mLoaded.store(true, std::memory_order_relaxed);
    } catch (...) {
        mLoaded.store(false, std::memory_order_relaxed);
    }
}

void NightVision::install() noexcept {
    if (mInstalled.load(std::memory_order_relaxed) || !mLoaded.load(std::memory_order_relaxed)) return;
    try {
        Hooks::hook();
        mInstalled.store(true, std::memory_order_relaxed);
    } catch (...) {
        mInstalled.store(false, std::memory_order_relaxed);
    }
}

void NightVision::uninstall() noexcept {
    if (!mInstalled.load(std::memory_order_relaxed)) return;
    mInstalled.store(false, std::memory_order_relaxed);
    try {
        Hooks::unhook();
    } catch (...) {
    }
    mState.setEnabled(false);
}

void NightVision::toggle(IClientInstance& client) {
    if (!mInstalled.load(std::memory_order_relaxed)) return;
    if (!lamina_view::isHudScreen(client.getScreenName())) return;
    mState.toggle();
    LaminaView::getInstance().getSelf().getLogger().debug("NightVision {}", mState.enabled() ? "on" : "off");
}

void NightVision::overrideProducedData(BaseLightData* lightData) {
    if (!lightData) return;
    bool const enabled = mState.enabled();
    if (enabled) {
        applyOverride(*lightData);
    }
#ifdef LAMINAVIEW_TRACE
    // Log only the transitions the render thread actually sees, so the
    // trace shows when the produced light data starts/stops being overridden
    // (the game rebuilds its light LUT on exactly those frames).
    bool const wasApplied = mTraceApplied.exchange(enabled, std::memory_order_relaxed);
    if (wasApplied != enabled) {
        LaminaView::getInstance().getSelf().getLogger().debug(
            "Light data override {} (nv={}/{:.2f} underwater={}/{:.2f} skyDarken={:.2f} gamma={:.2f})",
            enabled ? "applied" : "cleared",
            lightData->mNightvisionActive,
            lightData->mNightvisionScale,
            lightData->mUnderwaterVision,
            lightData->mUnderwaterScale,
            lightData->mSkyDarken,
            lightData->mGamma
        );
    }
#endif
}

void NightVision::applyOverride(BaseLightData& lightData) {
    // Mirror what the vanilla night-vision effect writes into the light
    // data: full night-vision scale with the previous-frame value matched so
    // there is no fade-in. Underwater is only ever brightened when the game
    // already flagged it: the underwater flag itself is never set, so above
    // water the tint path is untouched.
    lightData.mNightvisionActive = true;
    lightData.mNightvisionScale  = 1.0f;
    if (lightData.mUnderwaterVision) {
        lightData.mUnderwaterScale = 1.0f;
    }
    lightData.mDarkenWorldAmount           = 0.0f;
    lightData.mPreviousDarkenWorldAmount   = 0.0f;
    lightData.mDarknessFactor              = 0.0f;
    lightData.mDarknessFactorPreviousFrame = 0.0f;
}

} // namespace lamina_view::vision
