#include "AutomaticFishing.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/mod/RegisterHelper.h>
#include <ll/api/schedule/Scheduler.h>
#include <ll/api/schedule/Task.h>
#include <ll/api/service/Bedrock.h>
#include <mc/world/actor/FishingHook.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/Item.h>
#include <mc/world/level/Level.h>
#include <memory>

ll::schedule::GameTickScheduler tickScheduler;

LL_TYPE_INSTANCE_HOOK(PlayerPullFishingHook, HookPriority::Normal, FishingHook, &FishingHook::_serverHooked, bool) {
    auto rtn = origin();
    if (rtn && *((int*)this + 284) == 0) {
        auto  player = getPlayerOwner();
        auto& item   = player->getSelectedItem();
        item.getItem()->use(const_cast<ItemStack&>(item), *player);
        player->refreshInventory();
        auto& uid = player->getUuid();
        tickScheduler.add<ll::schedule::DelayTask>(ll::chrono_literals::operator""_tick(10), [uid]() {
            auto player = ll::service::getLevel()->getPlayer(uid);
            if (!player) return;
            auto& item = player->getSelectedItem();
            if (!item.isNull() && item.getTypeName() == "minecraft:fishing_rod") {
                item.getItem()->use(const_cast<ItemStack&>(item), *player);
            }
        });
    }
    return rtn;
}

namespace AutomaticFishing {


static std::unique_ptr<Entry> instance;

Entry& Entry::getInstance() { return *instance; }

bool Entry::load() { return true; }

bool Entry::enable() {
    ll::memory::HookRegistrar<PlayerPullFishingHook>().hook();
    return true;
}

bool Entry::disable() { return true; }

bool Entry::unload() {
    ll::memory::HookRegistrar<PlayerPullFishingHook>().unhook();
    return true;
}

} // namespace AutomaticFishing

LL_REGISTER_MOD(AutomaticFishing::Entry, AutomaticFishing::instance);
