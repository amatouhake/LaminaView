# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

### Fixed

- NightVision toggle is HUD-gated: key presses from chat, Creative search,
  anvil, inventory and other non-HUD screens are ignored, so typing never
  toggles brightness. Zoom wheel follows the universal convention (scroll up
  zooms in, scroll down zooms out).
- NightVision: client-side fullbright-style binary toggle (default `N`,
  remappable in the game's keyboard settings as `LaminaView.nightvision`).
  Brightens night, caves and dark interiors including underwater; writes
  nothing to the vanilla brightness option, injects no status effect, sends
  no packets. Disabling restores rendering exactly.
- Zoom: hold-to-zoom (default `C`, remappable as `LaminaView.zoom`), active
  only while held from the HUD screen. FOV-based with wheel-adjustable level
  (clamped), zoom-scaled look sensitivity, exact FOV/sensitivity restore on
  release. Never consumes the wheel when not zoomed; vanilla spyglass
  untouched. World unload/disconnect and dimension changes clear a stuck
  hold.
- Configuration file (`mods/LaminaView/config/config.json`) with independent
  `nightvision` / `zoom` sections and a `--trace=y` diagnostic build.
- Unit tests for the game-independent zoom math/state (`LaminaViewTests`).
