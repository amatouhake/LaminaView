#include "mod/LaminaView.h"

#include "ll/api/mod/RegisterHelper.h"

namespace lamina_view {

LaminaView& LaminaView::getInstance() {
    static LaminaView instance;
    return instance;
}

bool LaminaView::load() {
    getSelf().getLogger().debug("Loading...");
    return true;
}

bool LaminaView::enable() {
    getSelf().getLogger().debug("Enabling...");
    return true;
}

bool LaminaView::disable() {
    getSelf().getLogger().debug("Disabling...");
    return true;
}

} // namespace lamina_view

LL_REGISTER_MOD(lamina_view::LaminaView, lamina_view::LaminaView::getInstance());
