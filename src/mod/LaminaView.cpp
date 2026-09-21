#include "mod/LaminaView.h"

#include "mod/vision/NightVision.h"
#include "mod/zoom/Zoom.h"

#include "ll/api/Config.h"
#include "ll/api/io/FileSink.h"
#include "ll/api/io/LogLevel.h"
#include "ll/api/io/PatternFormatter.h"
#include "ll/api/io/RotatePolicy.h"
#include "ll/api/mod/RegisterHelper.h"

namespace lamina_view {

LaminaView& LaminaView::getInstance() {
    static LaminaView instance;
    return instance;
}

bool LaminaView::load() {
#ifdef LAMINAVIEW_TRACE
    // Trace builds mirror every line, flushed immediately, into the mod
    // directory so the diagnostics can be followed while the game runs.
    // Rotation is disabled so the file is simply appended to across runs.
    getSelf().getLogger().setLevel(ll::io::LogLevel::Debug);
    auto sink = std::make_shared<ll::io::FileSink>(
        getSelf().getModDir() / "trace.log",
        ll::makePolymorphic<ll::io::PatternFormatter>("[{3:.3%F %T.} {2}][{1}] {0}", false),
        ll::io::RotatePolicy::disabled()
    );
    sink->setFlushLevel(ll::io::LogLevel::Debug);
    getSelf().getLogger().addSink(std::move(sink));
    getSelf().getLogger().info("Trace build: debug logging enabled, mirrored to trace.log");
#endif
    getSelf().getLogger().debug("Loading...");

    // Missing file -> written with defaults; unknown/old version -> merged
    // with defaults and rewritten, so the file always reflects the schema.
    auto const configPath = getSelf().getConfigDir() / "config.json";
    try {
        if (!ll::config::loadConfig(mConfig, configPath)) {
            ll::config::saveConfig(mConfig, configPath);
        }
    } catch (std::exception const& e) {
        getSelf().getLogger().error("Failed to load {}: {}; using defaults", configPath.string(), e.what());
        mConfig = Config{};
    }
    getSelf().getLogger().debug(
        "Config: nightvision.enabled={} keyCode=0x{:X} zoom.enabled={} keyCode=0x{:X} level={}/{}/{} step={}",
        mConfig.nightvision.enabled,
        mConfig.nightvision.keyCode,
        mConfig.zoom.enabled,
        mConfig.zoom.keyCode,
        mConfig.zoom.defaultLevel,
        mConfig.zoom.minLevel,
        mConfig.zoom.maxLevel,
        mConfig.zoom.wheelStep
    );

    // The features are separable: each loads its own key/config behind a
    // noexcept boundary, so one failing must not break the other.
    vision::NightVision::getInstance().load(mConfig);
    zoom::Zoom::getInstance().load(mConfig);
    return true;
}

bool LaminaView::enable() {
    getSelf().getLogger().debug("Enabling...");
    vision::NightVision::getInstance().install();
    zoom::Zoom::getInstance().install();
    getSelf().getLogger().debug(
        "Enabled: nightvision(installed={} enabled={}) zoom(installed={} held={})",
        vision::NightVision::getInstance().installed(),
        vision::NightVision::getInstance().enabled(),
        zoom::Zoom::getInstance().installed(),
        zoom::Zoom::getInstance().held()
    );
    return true;
}

bool LaminaView::disable() {
    getSelf().getLogger().debug("Disabling...");
    // Reverse order of enable(); each uninstall is idempotent and restores
    // its own rendering/input state.
    zoom::Zoom::getInstance().uninstall();
    vision::NightVision::getInstance().uninstall();
    return true;
}

} // namespace lamina_view

LL_REGISTER_MOD(lamina_view::LaminaView, lamina_view::LaminaView::getInstance());
