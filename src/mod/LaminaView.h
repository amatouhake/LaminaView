#pragma once

#include "ll/api/mod/NativeMod.h"

#include "mod/Config.h"

namespace lamina_view {

class LaminaView {

public:
    static LaminaView& getInstance();

    LaminaView() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    [[nodiscard]] Config const& getConfig() const { return mConfig; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

private:
    ll::mod::NativeMod& mSelf;
    Config              mConfig;
};

} // namespace lamina_view
