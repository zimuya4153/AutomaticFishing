#include "AutomaticFishing.h"

#include <ll/api/memory/Hook.h>
#include <ll/api/mod/RegisterHelper.h>
#include <ll/api/schedule/Scheduler.h>
#include <ll/api/schedule/Task.h>
#include <ll/api/service/Bedrock.h>
#include <mc/nbt/CompoundTag.h>
#include <mc/world/actor/FishingHook.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/item/Item.h>
#include <mc/world/item/enchanting/Enchant.h>
#include <mc/world/item/enchanting/EnchantUtils.h>
#include <mc/world/level/Level.h>
#include <memory>


ll::schedule::GameTickScheduler tickScheduler;
std::map<FishingHook*, int>     fishingHook;

int randomInt() {
    static std::random_device                 rd;
    static std::default_random_engine         e(rd());
    static std::uniform_int_distribution<int> dist(0, 99);
    return dist(e);
}

void ReduceItemDurability(ItemStack& item, Player* player) {
    int durabilityLevel = EnchantUtils::getEnchantLevel(::Enchant::Type::MiningDurability, item);
    if (auto tag = item.save()->getCompound("tag")) {
        if (tag->getByte("Unbreakable")) {
            return;
        }
    }
    if (durabilityLevel == 0 || (durabilityLevel > 0 && randomInt() < (100 / durabilityLevel + 1))) {
        if (item.getMaxDamage() == item.getDamageValue() - 1) {
            item.setNull("");
            player->setSelectedItem(item);
            player->playSound(Puv::Legacy::LevelSoundEvent::ItemUseOn, player->getPosition(), 1);
            return;
        }
        item.setDamageValue(item.getDamageValue() + 1);
        player->setSelectedItem(item);
    }
}

LL_TYPE_INSTANCE_HOOK(PlayerPullFishingHook, HookPriority::Normal, FishingHook, &FishingHook::_updateServer, void) {
    origin();
    auto player = getPlayerOwner();
    if (_serverHooked() && !fishingHook.count(this)) {
        fishingHook[this] = 0;
        return;
    }
    if (_serverHooked() && fishingHook[this] == 0) {
        fishingHook[this] = 1;
        return;
    }
    if (!_serverHooked() && fishingHook[this] == 1) {
        auto item = player->getSelectedItem();
        item.getItem()->use(item, *player);
        fishingHook.erase(this);
        if (player->getPlayerGameType() != GameType::Creative) {
            ReduceItemDurability(item, player);
        }
        player->refreshInventory();
        auto& uid = player->getUuid();
        tickScheduler.add<ll::schedule::DelayTask>(ll::chrono_literals::operator""_tick(10), [uid]() {
            auto player = ll::service::getLevel()->getPlayer(uid);
            if (!player->isPlayer()) return;
            auto item = player->getSelectedItem();
            if (!item.isNull() && item.getTypeName() == "minecraft:fishing_rod") {
                item.getItem()->use(item, *player);
            }
        });
    }
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
