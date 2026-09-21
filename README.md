# LaminaView

A client-side NightVision + Zoom mod for Minecraft Bedrock Edition, built on
[LeviLamina Client](https://github.com/LiteLDev/LeviLamina).

LaminaView brightens dark scenes (NightVision) and magnifies the view while a
key is held (Zoom). Both are pure client-side visual features: no server
component, no cheats or commands, no status-effect injection.

## Target

* LeviLamina **v26.51.3** (client)
* Minecraft Bedrock Edition **1.26.51.x** (Windows x64)

## Current scope (MVP)

* **NightVision** – binary fullbright-style toggle (default `N`, remappable
  under *Settings → Keyboard & Mouse* as `LaminaView.nightvision`). HUD-gated:
  presses from any non-HUD screen (chat, Creative search, anvil, inventory,
  ...) belong to the UI and never toggle, so typing never flips brightness.
  Brightens night, caves and dark interiors; underwater keeps its tint but is
  brightened to match. Disabling restores the exact previous rendering.
* **Zoom** – hold-to-zoom (default `C`, remappable as `LaminaView.zoom`).
  Active only while held (and only from the HUD screen); mouse wheel adjusts
  the level while zoomed (scroll up zooms in); look sensitivity scales with
  the zoom level. Releasing restores FOV and sensitivity exactly. Zoom never
  steals the wheel from inventories or menus (opening a menu mid-hold drops
  the hold), and vanilla spyglass behaviour is unchanged. A stuck hold is
  impossible by construction: it clears on key-up, on any non-HUD screen, on
  app focus loss/suspend, and on world unload/disconnect/dimension change.

The two features are independent (separate state, keys, config and cleanup):
if one fails to initialise, the other still works.

## How it works

* `src/mod/vision/` hooks the shared `BaseLightTextureImageBuilder::buildImage`
  all dimensions render through, forcing night-vision light levels into the
  per-frame copy (underwater scale only raised when already flagged). The
  vanilla brightness option is never written, no effect is injected, and the
  hook is a pass-through when disabled.
* `src/mod/zoom/` narrows the camera FOV in `LevelRendererPlayer::setupCamera`
  while held and scales the `LocalPlayer::_applyTurnDelta` look delta by
  1/level (Vec2 yaw = x, pitch = z, compile-time asserted). Both hooks pass
  through untouched when not held; the hold clears on key-up, non-HUD screen,
  focus loss/suspend, world unload/disconnect and dimension change.

## Configuration

On first start LaminaView writes `mods/LaminaView/config/config.json` with

```json
{
    "version": 1,
    "nightvision": {
        "enabled": true,
        "keyCode": 78
    },
    "zoom": {
        "enabled": true,
        "keyCode": 67,
        "defaultLevel": 3.0,
        "minLevel": 1.5,
        "maxLevel": 10.0,
        "wheelStep": 0.5
    }
}
```

* `nightvision.enabled` / `zoom.enabled` – master switches. When `false`, the
  feature is inert (no key, no hook, no rendering change).
* `nightvision.keyCode` / `zoom.keyCode` – default Windows virtual-key codes
  (`78` is `N`, `67` is `C`). They only seed the game's keyboard settings;
  remap them in game.
* `zoom.defaultLevel` / `minLevel` / `maxLevel` – zoom factor bounds
  (a level of 3 means one third of the normal FOV).
* `zoom.wheelStep` – how much one wheel notch changes the level while zoomed.

The file is read once at mod load; restart the game after editing it.

For runtime diagnostics, configure with `--trace=y`; the mod then logs
NightVision/Zoom state transitions at debug level and mirrors them, flushed
immediately, to `mods/LaminaView/trace.log`.

Unit tests for the game-independent zoom math/state:

```shell
xmake build LaminaViewTests
xmake run LaminaViewTests
```

## Installation

Download `LaminaView-client-windows-x64.zip` from the
[GitHub Releases](https://github.com/amatouhake/LaminaView/releases) page and
copy the `LaminaView/` directory it contains (`LaminaView.dll` +
`manifest.json`) into the `mods/` directory of a LeviLamina client
installation (for a LeviLauncher instance:
`%APPDATA%\levilauncher.exe\versions\<version>\mods\LaminaView\`).

The repository also ships a `tooth.json`, so the release can be installed as
the LIP package `github.com/amatouhake/LaminaView` where LIP / LeviLauncher
package installation is available.

Keep the generated `manifest.json`: LeviLamina only loads mods whose manifest
says `"type": "native"`. Importing the bare DLL through LeviLauncher's
"import mod" dialog as `preload-native` produces a manifest LeviLamina ignores,
so the mod never loads.

LaminaView is early (`0.x`) software: it is usable, but its behaviour and
configuration may still change between releases.

## Building

Requirements: [xmake](https://xmake.io), Visual Studio 2022 build tools, and a
clang-cl toolchain (LLVM).

```shell
xmake f -y -p windows -a x64 -m release --target_type=client
xmake
```

The packaged mod (`LaminaView.dll` + `manifest.json`) is written to
`bin/LaminaView/`; install that folder as described under
[Installation](#installation). The `version` in the generated `manifest.json`
is derived from the nearest `vMAJOR.MINOR.PATCH` Git tag (`0.0.0` when there
is none).

Building against the LeviLamina SDK needs an LLVM that the SDK package can be
compiled with; the 26.51.x SDK was verified with LLVM 22.1.8 (`clang-cl`).
If the `levilamina` package fails to compile with the Visual Studio-bundled
LLVM, put a matching LLVM `bin` directory first on `PATH` for `xmake f`.

## Contributing

Ask questions by creating an issue. PRs accepted.

## License

[MIT](LICENSE) © amatouhake

Bootstrapped from the CC0-1.0 licensed
[levilamina-mod-template](https://github.com/LiteLDev/levilamina-mod-template).
