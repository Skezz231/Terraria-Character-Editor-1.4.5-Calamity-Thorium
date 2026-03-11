#pragma once
#include "Item.h"
#include <string>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <utility>

struct ColorRGBA { float r, g, b, a; };

struct PlayerData {
    // Основное
    std::string name = "Player";
    int difficulty = 0;                       // 0=classic,1=expert,2=master,3=journey
    int hairStyle = 0;                        // 0–145+ (новые 60+ из 1.4.5)
    int skinVariant = 0;
    
    ColorRGBA hairColor{1,1,1,1}, skinColor{1,1,1,1}, eyeColor{1,1,1,1};
    ColorRGBA shirtColor{1,1,1,1}, undershirtColor{1,1,1,1}, pantsColor{1,1,1,1}, shoeColor{1,1,1,1};

    int health = 100, maxHealth = 100;
    int mana = 20, maxMana = 20;

    // Инвентарь и хранилища
    std::vector<Item> inventory;     // 50
    std::vector<Item> armor;         // 20 (3 armor + 5 acc + vanity/social/dyes)
    std::vector<Item> dye;           // 10
    std::vector<Item> ammo;          // 4
    std::vector<Item> coins;         // 4

    std::vector<Item> piggyBank;     // 40
    std::vector<Item> safe;          // 40
    std::vector<Item> forge;         // 40
    std::vector<Item> voidVault;     // 50

    // Buffs & Transformations
    std::vector<std::pair<int, float>> buffs;     // id баффа → время (сек)
    std::vector<int> activeTransformations;       // Bat form, Rat form и т.д.

    // Journey
    std::unordered_map<int, int> journeyResearch; // itemId → research level

    // Дополнительно из 1.4.5+
    bool   isSkyblockStart     = false;   // флаг для Skyblock seed
    int    spawnX = 0, spawnY = 0;
    bool   hasDeadCellsVanity  = false;   // Beheaded set и т.д.
    int    ramRuneCharges      = 0;       // Dead Cells accessory

    PlayerData() {
        ensureSizes();
    }

    void ensureSizes() {
        inventory.resize(50);
        armor.resize(20);
        dye.resize(10);
        ammo.resize(4);
        coins.resize(4);
        piggyBank.resize(40);
        safe.resize(40);
        forge.resize(40);
        voidVault.resize(50);
    }

    int filledSlotCount() const {
        const auto countFilled = [](const std::vector<Item>& slots) {
            return static_cast<int>(std::count_if(slots.begin(), slots.end(), [](const Item& item) { return !item.isEmpty(); }));
        };

        return countFilled(inventory)
            + countFilled(armor)
            + countFilled(dye)
            + countFilled(ammo)
            + countFilled(coins)
            + countFilled(piggyBank)
            + countFilled(safe)
            + countFilled(forge)
            + countFilled(voidVault);
    }

    int totalStackCount() const {
        const auto countStacks = [](const std::vector<Item>& slots) {
            int total = 0;
            for (const auto& item : slots) {
                if (!item.isEmpty()) {
                    total += std::max(item.stack, 0);
                }
            }
            return total;
        };

        return countStacks(inventory)
            + countStacks(armor)
            + countStacks(dye)
            + countStacks(ammo)
            + countStacks(coins)
            + countStacks(piggyBank)
            + countStacks(safe)
            + countStacks(forge)
            + countStacks(voidVault);
    }
};
