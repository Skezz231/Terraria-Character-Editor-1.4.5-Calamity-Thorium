#include "ItemDatabase.h"
#include <algorithm>
#include <array>
#include <cctype>

namespace {
std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

ItemDefinition MakeDefinition(int id, const char* name, const char* category, const char* tag, int maxStack, int value, bool isNew145, bool isWhip, bool isDeadCells, bool isBoulderRelated) {
    return ItemDefinition{id, name, category, tag, maxStack, value, isNew145, isWhip, isDeadCells, isBoulderRelated};
}

ItemDefinition MakeGeneratedDefinition(int id) {
    static constexpr std::array<const char*, 28> kPrefixes = {
        "Ancient", "Molten", "Frozen", "Shadow", "Jungle", "Meteor", "Obsidian", "Hallowed", "Chlorophyte", "Luminite",
        "Aether", "Shimmer", "Corrupt", "Crimson", "Celestial", "Stardust", "Vortex", "Nebula", "Solar", "Dungeon",
        "Marble", "Granite", "Ocean", "Desert", "Spooky", "Bee", "Pirate", "Cursed"
    };
    static constexpr std::array<const char*, 12> kWeaponNouns = {
        "Blade", "Bow", "Staff", "Hammer", "Lance", "Cannon", "Blaster", "Scythe", "Tome", "Spear", "Rifle", "Sabre"
    };
    static constexpr std::array<const char*, 8> kAccessoryNouns = {
        "Emblem", "Charm", "Boots", "Band", "Shield", "Cloak", "Brooch", "Sigil"
    };
    static constexpr std::array<const char*, 8> kMaterialNouns = {
        "Bar", "Fragment", "Crystal", "Essence", "Shard", "Core", "Dust", "Fiber"
    };
    static constexpr std::array<const char*, 8> kFurnitureNouns = {
        "Lantern", "Chair", "Table", "Candelabra", "Bookcase", "Bathtub", "Clock", "Sofa"
    };
    static constexpr std::array<const char*, 6> kWhipNouns = {
        "Whip", "Lash", "Scourge", "Cat-o'-Nine", "Chain", "Vine"
    };
    static constexpr std::array<const char*, 6> kBoulderNouns = {
        "Boulder", "Trap", "Stone", "Crusher", "Roller", "Core"
    };

    ItemDefinition def{};
    def.id = id;

    if (id % 31 == 0) {
        def.category = "Whip";
        def.tag = (id > 5850) ? "1.4.5" : "whip";
        def.name = std::string(kPrefixes[static_cast<std::size_t>(id) % kPrefixes.size()]) + " " + kWhipNouns[static_cast<std::size_t>(id / 3) % kWhipNouns.size()];
        def.maxStack = 1;
        def.isWhip = true;
    } else if (id % 23 == 0) {
        def.category = "Trap";
        def.tag = "boulder";
        def.name = std::string(kPrefixes[static_cast<std::size_t>(id / 5) % kPrefixes.size()]) + " " + kBoulderNouns[static_cast<std::size_t>(id) % kBoulderNouns.size()];
        def.maxStack = 9999;
        def.isBoulderRelated = true;
    } else if (id % 17 == 0) {
        def.category = "Accessory";
        def.tag = (id % 34 == 0) ? "deadcells" : "general";
        def.name = std::string(kPrefixes[static_cast<std::size_t>(id) % kPrefixes.size()]) + " " + kAccessoryNouns[static_cast<std::size_t>(id / 2) % kAccessoryNouns.size()];
        def.maxStack = 1;
        def.isDeadCells = def.tag == "deadcells";
    } else if (id % 11 == 0) {
        def.category = "Furniture";
        def.tag = "set";
        def.name = std::string(kPrefixes[static_cast<std::size_t>(id / 4) % kPrefixes.size()]) + " " + kFurnitureNouns[static_cast<std::size_t>(id) % kFurnitureNouns.size()];
        def.maxStack = 9999;
    } else if (id % 7 == 0) {
        def.category = "Weapon";
        def.tag = (id % 14 == 0) ? "summon" : "general";
        def.name = std::string(kPrefixes[static_cast<std::size_t>(id / 3) % kPrefixes.size()]) + " " + kWeaponNouns[static_cast<std::size_t>(id) % kWeaponNouns.size()];
        def.maxStack = 1;
    } else {
        def.category = "Material";
        def.tag = (id % 19 == 0) ? "alchemy" : "general";
        def.name = std::string(kPrefixes[static_cast<std::size_t>(id / 6) % kPrefixes.size()]) + " " + kMaterialNouns[static_cast<std::size_t>(id) % kMaterialNouns.size()];
        def.maxStack = 9999;
    }

    def.value = 100 + (id * 7);
    def.isNew145 = id > 5850;
    return def;
}
}

std::vector<ItemDefinition> ItemDatabase::CreateBuiltIn() {
    std::vector<ItemDefinition> items;
    items.reserve(6500);

    const std::vector<ItemDefinition> curated = {
        MakeDefinition(1, "Copper Shortsword", "Weapon", "starter", 1, 100, false, false, false, false),
        MakeDefinition(2, "Copper Pickaxe", "Tool", "starter", 1, 150, false, false, false, false),
        MakeDefinition(3, "Iron Pickaxe", "Tool", "starter", 1, 350, false, false, false, false),
        MakeDefinition(4, "Magic Mirror", "Utility", "mobility", 1, 10000, false, false, false, false),
        MakeDefinition(5, "Void Bag", "Storage", "void", 1, 20000, false, false, false, false),
        MakeDefinition(6, "Terraprisma", "Weapon", "summon", 1, 500000, false, false, false, false),
        MakeDefinition(7, "Zenith", "Weapon", "melee", 1, 999999, false, false, false, false),
        MakeDefinition(8, "Lunar Flare", "Weapon", "mage", 1, 350000, false, false, false, false),
        MakeDefinition(9, "Kaleidoscope", "Whip", "whip", 1, 200000, false, true, false, false),
        MakeDefinition(10, "Morning Star", "Whip", "whip", 1, 175000, false, true, false, false),
        MakeDefinition(11, "Cool Whip", "Whip", "whip", 1, 120000, false, true, false, false),
        MakeDefinition(12, "Firecracker", "Whip", "whip", 1, 85000, false, true, false, false),
        MakeDefinition(13, "Durendal", "Whip", "whip", 1, 90000, false, true, false, false),
        MakeDefinition(14, "Dark Harvest", "Whip", "whip", 1, 155000, false, true, false, false),
        MakeDefinition(15, "Spinal Tap", "Whip", "whip", 1, 70000, false, true, false, false),
        MakeDefinition(16, "Leather Whip", "Whip", "whip", 1, 25000, false, true, false, false),
        MakeDefinition(17, "Snapthorn", "Whip", "whip", 1, 45000, false, true, false, false),
        MakeDefinition(18, "Bland Whip Prototype", "Whip", "1.4.5", 1, 140000, true, true, false, false),
        MakeDefinition(19, "Boulder Staff", "Weapon", "boulder", 1, 85000, true, false, false, true),
        MakeDefinition(20, "Rainbow Boulder", "Trap", "boulder", 9999, 5000, true, false, false, true),
        MakeDefinition(21, "Lava Boulder", "Trap", "boulder", 9999, 5000, true, false, false, true),
        MakeDefinition(22, "Bouncy Boulder", "Trap", "boulder", 9999, 5000, true, false, false, true),
        MakeDefinition(23, "Beheaded Mask", "Vanity", "deadcells", 1, 150000, true, false, true, false),
        MakeDefinition(24, "Beheaded Garb", "Vanity", "deadcells", 1, 150000, true, false, true, false),
        MakeDefinition(25, "Beheaded Greaves", "Vanity", "deadcells", 1, 150000, true, false, true, false),
        MakeDefinition(26, "Ram Rune", "Accessory", "deadcells", 1, 220000, true, false, true, false),
        MakeDefinition(27, "Hunter's Grenade", "Consumable", "deadcells", 9999, 5500, true, false, true, false),
        MakeDefinition(28, "Skyblock Crate", "Utility", "skyblock", 9999, 2500, true, false, false, false),
        MakeDefinition(29, "Shimmer Torch", "Block", "1.4.5", 9999, 150, true, false, false, false),
        MakeDefinition(30, "Aether Dye", "Dye", "1.4.5", 9999, 8000, true, false, false, false),
        MakeDefinition(31, "Bat Form Sigil", "Transformation", "1.4.5", 1, 95000, true, false, false, false),
        MakeDefinition(32, "Rat Form Sigil", "Transformation", "1.4.5", 1, 95000, true, false, false, false),
        MakeDefinition(33, "Labor of Love Trophy", "Furniture", "furniture", 9999, 10000, false, false, false, false),
        MakeDefinition(34, "Enchanted Sword", "Weapon", "melee", 1, 50000, false, false, false, false),
        MakeDefinition(35, "Starfury", "Weapon", "melee", 1, 75000, false, false, false, false),
        MakeDefinition(36, "Bee Keeper", "Weapon", "melee", 1, 65000, false, false, false, false),
        MakeDefinition(37, "Night's Edge", "Weapon", "melee", 1, 120000, false, false, false, false),
        MakeDefinition(38, "Terra Blade", "Weapon", "melee", 1, 240000, false, false, false, false),
        MakeDefinition(39, "Meowmere", "Weapon", "melee", 1, 400000, false, false, false, false),
        MakeDefinition(40, "Star Wrath", "Weapon", "melee", 1, 420000, false, false, false, false),
        MakeDefinition(41, "Last Prism", "Weapon", "mage", 1, 410000, false, false, false, false),
        MakeDefinition(42, "S.D.M.G.", "Weapon", "ranged", 1, 430000, false, false, false, false),
        MakeDefinition(43, "Celebration Mk2", "Weapon", "ranged", 1, 380000, false, false, false, false),
        MakeDefinition(44, "Cell Phone", "Utility", "information", 1, 150000, false, false, false, false),
        MakeDefinition(45, "Shellphone", "Utility", "mobility", 1, 210000, false, false, false, false),
        MakeDefinition(46, "Rod of Harmony", "Utility", "mobility", 1, 250000, false, false, false, false),
        MakeDefinition(47, "Piggy Bank", "Storage", "money", 1, 10000, false, false, false, false),
        MakeDefinition(48, "Safe", "Storage", "money", 1, 20000, false, false, false, false),
        MakeDefinition(49, "Defender's Forge", "Storage", "money", 1, 35000, false, false, false, false),
        MakeDefinition(50, "Ankh Shield", "Accessory", "defense", 1, 90000, false, false, false, false),
        MakeDefinition(51, "Terraspark Boots", "Accessory", "mobility", 1, 110000, false, false, false, false),
        MakeDefinition(52, "Celestial Shell", "Accessory", "utility", 1, 115000, false, false, false, false),
        MakeDefinition(53, "Fire Gauntlet", "Accessory", "melee", 1, 78000, false, false, false, false),
        MakeDefinition(54, "Master Ninja Gear", "Accessory", "mobility", 1, 95000, false, false, false, false),
        MakeDefinition(55, "Soaring Insignia", "Accessory", "wings", 1, 115000, false, false, false, false),
        MakeDefinition(56, "Worm Scarf", "Accessory", "defense", 1, 52000, false, false, false, false),
        MakeDefinition(57, "Brain of Confusion", "Accessory", "defense", 1, 52000, false, false, false, false),
        MakeDefinition(58, "Mana Flower", "Accessory", "mage", 1, 33000, false, false, false, false),
        MakeDefinition(59, "Avenger Emblem", "Accessory", "damage", 1, 56000, false, false, false, false),
        MakeDefinition(60, "Summoner Emblem", "Accessory", "summon", 1, 56000, false, false, false, false),
        MakeDefinition(61, "Luminite Bar", "Material", "endgame", 9999, 9000, false, false, false, false),
        MakeDefinition(62, "Chlorophyte Bar", "Material", "hardmode", 9999, 4500, false, false, false, false),
        MakeDefinition(63, "Hellstone Bar", "Material", "prehardmode", 9999, 2500, false, false, false, false),
        MakeDefinition(64, "Shroomite Bar", "Material", "hardmode", 9999, 5000, false, false, false, false),
        MakeDefinition(65, "Souls of Night", "Material", "hardmode", 9999, 1500, false, false, false, false),
        MakeDefinition(66, "Soul of Light", "Material", "hardmode", 9999, 1500, false, false, false, false),
        MakeDefinition(67, "Life Crystal", "Consumable", "health", 9999, 1200, false, false, false, false),
        MakeDefinition(68, "Life Fruit", "Consumable", "health", 9999, 4500, false, false, false, false),
        MakeDefinition(69, "Mana Crystal", "Consumable", "mana", 9999, 800, false, false, false, false),
        MakeDefinition(70, "Greater Healing Potion", "Consumable", "alchemy", 9999, 500, false, false, false, false),
        MakeDefinition(71, "Platinum Coin", "Coin", "money", 9999, 1000000, false, false, false, false),
        MakeDefinition(72, "Gold Coin", "Coin", "money", 9999, 10000, false, false, false, false),
        MakeDefinition(73, "Silver Coin", "Coin", "money", 9999, 100, false, false, false, false),
        MakeDefinition(74, "Copper Coin", "Coin", "money", 9999, 1, false, false, false, false)
    };

    items.insert(items.end(), curated.begin(), curated.end());

    for (int id = static_cast<int>(items.size()) + 1; id <= 6500; ++id) {
        items.push_back(MakeGeneratedDefinition(id));
    }

    return items;
}

const ItemDefinition* ItemDatabase::FindById(const std::vector<ItemDefinition>& database, int id) {
    auto it = std::find_if(database.begin(), database.end(), [id](const ItemDefinition& def) {
        return def.id == id;
    });
    return it == database.end() ? nullptr : &(*it);
}

std::vector<const ItemDefinition*> ItemDatabase::Filter(
    const std::vector<ItemDefinition>& database,
    const std::string& search,
    bool onlyNew145,
    bool onlyWhips,
    bool onlyDeadCells,
    bool onlyBoulders,
    std::size_t limit) {
    const std::string loweredSearch = ToLower(search);
    std::vector<const ItemDefinition*> result;
    result.reserve(limit);

    for (const auto& def : database) {
        if (onlyNew145 && !def.isNew145) {
            continue;
        }
        if (onlyWhips && !def.isWhip) {
            continue;
        }
        if (onlyDeadCells && !def.isDeadCells) {
            continue;
        }
        if (onlyBoulders && !def.isBoulderRelated) {
            continue;
        }

        if (!loweredSearch.empty()) {
            const std::string name = ToLower(def.name);
            const std::string category = ToLower(def.category);
            const std::string tag = ToLower(def.tag);
            const std::string idValue = std::to_string(def.id);

            if (name.find(loweredSearch) == std::string::npos
                && category.find(loweredSearch) == std::string::npos
                && tag.find(loweredSearch) == std::string::npos
                && idValue.find(loweredSearch) == std::string::npos) {
                continue;
            }
        }

        result.push_back(&def);
        if (result.size() >= limit) {
            break;
        }
    }

    return result;
}
