#include "PlayerSerialization.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

void to_json(json& j, const ColorRGBA& color) {
    j = json{{"r", color.r}, {"g", color.g}, {"b", color.b}, {"a", color.a}};
}

void from_json(const json& j, ColorRGBA& color) {
    color.r = j.value("r", 1.0f);
    color.g = j.value("g", 1.0f);
    color.b = j.value("b", 1.0f);
    color.a = j.value("a", 1.0f);
}

void to_json(json& j, const Item& item) {
    j = json{{"id", item.id}, {"stack", item.stack}, {"prefix", item.prefix}, {"favorite", item.favorite}, {"locked", item.locked}, {"modDataSize", item.modDataSize}};
}

void from_json(const json& j, Item& item) {
    item.id = j.value("id", 0);
    item.stack = j.value("stack", 1);
    item.prefix = j.value("prefix", static_cast<std::uint8_t>(0));
    item.favorite = j.value("favorite", false);
    item.locked = j.value("locked", false);
    item.modDataSize = j.value("modDataSize", 0);
}

void to_json(json& j, const RecentPlayerEntry& entry) {
    j = json{{"name", entry.name}, {"path", entry.path}, {"timestamp", entry.timestamp}};
}

void from_json(const json& j, RecentPlayerEntry& entry) {
    entry.name = j.value("name", std::string{});
    entry.path = j.value("path", std::string{});
    entry.timestamp = j.value("timestamp", std::string{});
}

namespace {
json PlayerToJson(const PlayerData& player) {
    return json{
        {"name", player.name},
        {"difficulty", player.difficulty},
        {"hairStyle", player.hairStyle},
        {"skinVariant", player.skinVariant},
        {"hairColor", player.hairColor},
        {"skinColor", player.skinColor},
        {"eyeColor", player.eyeColor},
        {"shirtColor", player.shirtColor},
        {"undershirtColor", player.undershirtColor},
        {"pantsColor", player.pantsColor},
        {"shoeColor", player.shoeColor},
        {"health", player.health},
        {"maxHealth", player.maxHealth},
        {"mana", player.mana},
        {"maxMana", player.maxMana},
        {"inventory", player.inventory},
        {"armor", player.armor},
        {"dye", player.dye},
        {"ammo", player.ammo},
        {"coins", player.coins},
        {"piggyBank", player.piggyBank},
        {"safe", player.safe},
        {"forge", player.forge},
        {"voidVault", player.voidVault},
        {"buffs", player.buffs},
        {"activeTransformations", player.activeTransformations},
        {"journeyResearch", player.journeyResearch},
        {"isSkyblockStart", player.isSkyblockStart},
        {"spawnX", player.spawnX},
        {"spawnY", player.spawnY},
        {"hasDeadCellsVanity", player.hasDeadCellsVanity},
        {"ramRuneCharges", player.ramRuneCharges}
    };
}

void PlayerFromJson(const json& j, PlayerData& player) {
    player.name = j.value("name", std::string{"Player"});
    player.difficulty = j.value("difficulty", 0);
    player.hairStyle = j.value("hairStyle", 0);
    player.skinVariant = j.value("skinVariant", 0);
    player.hairColor = j.value("hairColor", ColorRGBA{1, 1, 1, 1});
    player.skinColor = j.value("skinColor", ColorRGBA{1, 1, 1, 1});
    player.eyeColor = j.value("eyeColor", ColorRGBA{1, 1, 1, 1});
    player.shirtColor = j.value("shirtColor", ColorRGBA{1, 1, 1, 1});
    player.undershirtColor = j.value("undershirtColor", ColorRGBA{1, 1, 1, 1});
    player.pantsColor = j.value("pantsColor", ColorRGBA{1, 1, 1, 1});
    player.shoeColor = j.value("shoeColor", ColorRGBA{1, 1, 1, 1});
    player.health = j.value("health", 100);
    player.maxHealth = j.value("maxHealth", 100);
    player.mana = j.value("mana", 20);
    player.maxMana = j.value("maxMana", 20);
    player.inventory = j.value("inventory", std::vector<Item>{});
    player.armor = j.value("armor", std::vector<Item>{});
    player.dye = j.value("dye", std::vector<Item>{});
    player.ammo = j.value("ammo", std::vector<Item>{});
    player.coins = j.value("coins", std::vector<Item>{});
    player.piggyBank = j.value("piggyBank", std::vector<Item>{});
    player.safe = j.value("safe", std::vector<Item>{});
    player.forge = j.value("forge", std::vector<Item>{});
    player.voidVault = j.value("voidVault", std::vector<Item>{});
    player.buffs = j.value("buffs", std::vector<std::pair<int, float>>{});
    player.activeTransformations = j.value("activeTransformations", std::vector<int>{});
    player.journeyResearch = j.value("journeyResearch", std::unordered_map<int, int>{});
    player.isSkyblockStart = j.value("isSkyblockStart", false);
    player.spawnX = j.value("spawnX", 0);
    player.spawnY = j.value("spawnY", 0);
    player.hasDeadCellsVanity = j.value("hasDeadCellsVanity", false);
    player.ramRuneCharges = j.value("ramRuneCharges", 0);
    player.ensureSizes();
}
}

namespace PlayerSerialization {
bool SavePlayer(const PlayerData& player, const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << std::setw(2) << PlayerToJson(player);
    return static_cast<bool>(out);
}

bool LoadPlayer(PlayerData& player, const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }

    json j;
    in >> j;
    PlayerFromJson(j, player);
    return true;
}

std::vector<RecentPlayerEntry> LoadRecentPlayers(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }

    json j;
    in >> j;
    return j.get<std::vector<RecentPlayerEntry>>();
}

bool SaveRecentPlayers(const std::vector<RecentPlayerEntry>& entries, const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << std::setw(2) << json(entries);
    return static_cast<bool>(out);
}

void TouchRecentPlayer(std::vector<RecentPlayerEntry>& entries, const std::string& name, const std::filesystem::path& playerPath) {
    const std::string normalizedPath = playerPath.generic_string();
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&normalizedPath](const RecentPlayerEntry& entry) {
        return entry.path == normalizedPath;
    }), entries.end());

    entries.insert(entries.begin(), RecentPlayerEntry{name, normalizedPath, TimestampNow()});
    if (entries.size() > 8) {
        entries.resize(8);
    }
}

std::string TimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}
}
