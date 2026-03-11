#pragma once
#include "PlayerData.h"
#include <filesystem>
#include <string>
#include <vector>

struct RecentPlayerEntry {
    std::string name;
    std::string path;
    std::string timestamp;
};

namespace PlayerSerialization {
    bool SavePlayer(const PlayerData& player, const std::filesystem::path& path);
    bool LoadPlayer(PlayerData& player, const std::filesystem::path& path);

    std::vector<RecentPlayerEntry> LoadRecentPlayers(const std::filesystem::path& path);
    bool SaveRecentPlayers(const std::vector<RecentPlayerEntry>& entries, const std::filesystem::path& path);
    void TouchRecentPlayer(std::vector<RecentPlayerEntry>& entries, const std::string& name, const std::filesystem::path& playerPath);

    std::string TimestampNow();
}
