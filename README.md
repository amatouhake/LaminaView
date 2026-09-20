# LaminaView

A client-side NightVision + Zoom mod for Minecraft Bedrock Edition, built on
[LeviLamina Client](https://github.com/LiteLDev/LeviLamina).

LaminaView will brighten dark scenes (NightVision, fullbright-style toggle)
and magnify the view while a key is held (Zoom). Both are pure client-side
visual features: no server component, no cheats or commands, no status-effect
injection.

## Target

* LeviLamina **v26.51.3** (client)
* Minecraft Bedrock Edition **1.26.51.x** (Windows x64)

This is a pure client mod. It does not need any server-side component and it
never modifies inventories, sends transactions, or alters packets.

## Current scope (MVP)

Planned (not yet implemented; landing on `feat/nightvision-zoom`):

* **NightVision** – binary toggle that brightens night, caves and dark
  interiors, remappable through the game's keyboard settings.
* **Zoom** – hold-to-zoom with wheel-adjustable level and zoom-scaled look
  sensitivity, remappable through the game's keyboard settings.

## Building

Requirements: [xmake](https://xmake.io), Visual Studio 2022 build tools, and a
clang-cl toolchain (LLVM).

```shell
xmake f -y -p windows -a x64 -m release --target_type=client
xmake
```

The packaged mod (`LaminaView.dll` + `manifest.json`) is written to
`bin/LaminaView/`. Copy that folder into the `mods/` directory of a LeviLamina
client installation.

## Contributing

Ask questions by creating an issue. PRs accepted.

## License

[MIT](LICENSE) © amatouhake

Bootstrapped from the CC0-1.0 licensed
[levilamina-mod-template](https://github.com/LiteLDev/levilamina-mod-template).
