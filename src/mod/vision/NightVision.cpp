#include "mod/vision/NightVision.h"

#include "mod/Config.h"
#include "mod/LaminaView.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/client/ClientExitLevelEvent.h"
#include "ll/api/input/KeyRegistry.h"
#include "ll/api/memory/Hook.h"

#include "mc/client/renderer/ptexture/BaseLightData.h"
#include "mc/client/renderer/ptexture/BaseLightTextureImageBuilder.h"
#include "mc/deps/core/image/Image.h"

namespace lamina_view::vision {

namespace {

constexpr std::string_view kKeyName = "nightvision";

// Hook the shared base implementation so Overworld, Nether, The End and any
// custom dimension pick up the override through the same virtual.
LL_TYPE_INSTANCE_HOOK(
    BuildImageHook,
    ll::memory::HookPriority::Normal,
    BaseLightTextureImageBuilder,
    &BaseLightTextureImageBuilder::$buildImage,
    bool,
    ::BaseLightData const& lightData,
    ::mce::Image*          targetImage,
    uint                   imageLength,
    float                  a,
    float                  ambientBoost,
    bool                   clampToMinimum
) {
    auto& self = NightVision::getInstance();
    if (!self.enabled()) {
        return origin(lightData, targetImage, imageLength, a, ambientBoost, clampToMinimum);
    }
    // The input is const, but the pipeline only reads it to rasterize the
    // 16x16 light LUT for this frame; adjusting the copy is a pure visual
    // override with no effect on saved options or the world.
    ::BaseLightData overridden = lightData;
    NightVision::applyOverride(overridden);
    return origin(overridden, targetImage, imageLength, a, ambientBoost, clampToMinimum);
}

using Hooks = ll::memory::HookRegistrar<BuildImageHook>;

} // namespace

NightVision& NightVision::getInstance() {
    static NightVision instance;
    return instance;
}

void NightVision::load(Config const& config) noexcept {
    mOwnedConfig = std::make_unique<Config>(config);
    if (!config.nightvision.enabled) return;
    try {
        auto& key =
            ll::input::KeyRegistry::getInstance().getOrCreateKey(kKeyName, {config.nightvision.keyCode});
        key.registerButtonDownHandler([this](::FocusImpact, ::IClientInstance&) { toggle(); });
        mLoaded = true;
    } catch (...) {
        mLoaded = false;
    }
}

void NightVision::install() noexcept {
    if (mInstalled || !mLoaded) return;
    try {
        Hooks::hook();
        mExitListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::ClientExitLevelEvent>(
            [this](ll::event::ClientExitLevelEvent&) { onWorldLeft(); }
        );
        mInstalled = true;
    } catch (...) {
        mInstalled = false;
    }
}

void NightVision::uninstall() noexcept {
    if (!mInstalled) return;
    try {
        if (mExitListener) {
            ll::event::EventBus::getInstance().removeListener(mExitListener);
            mExitListener.reset();
        }
        Hooks::unhook();
    } catch (...) {
    }
    mState.setEnabled(false);
    mInstalled = false;
}

void NightVision::toggle() {
    if (!mInstalled) return;
    mState.toggle();
    LaminaView::getInstance().getSelf().getLogger().debug("NightVision {}", mState.enabled() ? "on" : "off");
}

void NightVision::applyOverride(BaseLightData& lightData) {
    // Mirror what the vanilla night-vision effect writes into the light
    // data: full night-vision scale with the previous-frame value matched so
    // there is no fade-in, underwater brightened to the same scale so caves
    // under water stay readable without changing the water tint itself.
    lightData.mNightvisionActive         = true;
    lightData.mNightvisionScale          = 1.0f;
    lightData.mUnderwaterVision          = true;
    lightData.mUnderwaterScale           = 1.0f;
    lightData.mDarkenWorldAmount         = 0.0f;
    lightData.mPreviousDarkenWorldAmount = 0.0f;
    lightData.mDarknessFactor            = 0.0f;
    lightData.mDarknessFactorPreviousFrame = 0.0f;
}

} // namespace lamina_view::vision
