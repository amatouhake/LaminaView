#pragma once

namespace lamina_view {

/// Persistent settings, stored as JSON in the mod's config directory.
/// Bump `version` when the layout changes incompatibly.
///
/// NightVision and Zoom keep separate sections (state, keys, config and
/// cleanup are independent per feature), so one failing must not break the
/// other.
struct Config {
    int version = 1;

    struct NightVision {
        /// Master switch. When false NightVision installs nothing.
        bool enabled = true;
        /// Default key code for the NightVision toggle (Windows
        /// virtual-key code; 0x4E is 'N'). Seeds the game's keyboard
        /// settings; remappable there.
        int keyCode = 0x4E;
    } nightvision;

    struct Zoom {
        /// Master switch. When false Zoom installs nothing.
        bool enabled = true;
        /// Default key code for hold-to-zoom (Windows virtual-key code;
        /// 0x43 is 'C'). Seeds the game's keyboard settings.
        int keyCode = 0x43;
        /// Zoom factor applied while held (3 = one third of normal FOV).
        float defaultLevel = 3.0f;
        /// Zoom factor bounds.
        float minLevel = 1.5f;
        float maxLevel = 10.0f;
        /// Level change per mouse-wheel notch while zoomed.
        float wheelStep = 0.5f;
    } zoom;
};

} // namespace lamina_view
