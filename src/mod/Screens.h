#pragma once

#include <string>

namespace lamina_view {

/// Only the HUD screen leaves hotkeys and the wheel to gameplay; everywhere
/// else (inventory, chest, pause, chat, ...) input belongs to the UI. Compared
/// by prefix so suffixed variants ("hud_screen_lite", ...) behave the same.
/// Post-remap gate: the key stays remappable in game settings; the handler
/// just ignores presses from non-HUD screens.
inline bool isHudScreen(std::string const& screenName) { return screenName.rfind("hud_screen", 0) == 0; }

} // namespace lamina_view
