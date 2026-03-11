#pragma once
#include <imgui.h>
#include "ItemDatabase.h"
#include "PlayerData.h"
#include "PlayerSerialization.h"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

class AppUI {
public:
    AppUI();
    ~AppUI();

    void Init();
    void Render();

private:
    enum class ThemePreset {
        Dark2026,
        Classic13,
        LaborOfLove,
        BoulderGlow
    };

    struct SlotPayload {
        Item* source = nullptr;
    };

    struct SpawnPayload {
        int itemId = 0;
        int stack = 1;
    };

    PlayerData player;
    std::vector<ItemDefinition> itemDatabase;
    std::vector<RecentPlayerEntry> recentPlayers;

    std::filesystem::path appDataDir;
    std::filesystem::path terrariaPlayersDirectory;
    std::filesystem::path terrariaExecutablePath;
    ThemePreset activeTheme = ThemePreset::Dark2026;

    std::array<char, 128> playerNameBuffer{};
    std::array<char, 128> searchBuffer{};
    std::array<char, 128> presetNameBuffer{};
    std::array<char, 260> iconPackBuffer{};

    std::string statusMessage;
    float uiScale = 1.0f;
    float appliedUiScale = 1.0f;
    int browserSpawnStack = 999;
    int selectedDefinitionId = 6;
    int selectedBuffId = 1;
    float selectedBuffTime = 600.0f;
    int selectedTransformation = 0;
    int selectedTerrariaPlayerIndex = -1;
    double nextIntegrationProbeTime = 0.0;

    bool filterNew145 = false;
    bool filterWhips = false;
    bool filterDeadCells = false;
    bool filterBoulders = false;
    bool compactSlots = false;
    bool layoutInitialized = false;

    void RenderDockspace();
    void BuildDefaultDockLayout(ImGuiID dockspaceId);
    void RenderMenuBar();
    void RenderDashboard();
    void RenderInventory();
    void RenderEquipment();
    void RenderStorage();
    void RenderCharacterCreator();
    void RenderBuffsEffects();
    void RenderJourneyResearch();
    void RenderItemBrowser();
    void RenderWorldFlags();
    void RenderCheatsFun();
    void RenderSettingsThemes();

    void RenderItemGrid(const char* id, std::vector<Item>& items, int columns, int idBase);
    void DrawItemSlot(Item& item, int index, const char* labelPrefix);
    void DrawPlayerPreview();

    void SeedDemoPlayer();
    void SyncBuffersFromPlayer();
    void ApplyTheme(ThemePreset preset);
    void SaveCurrentPlayer();
    void LoadCurrentPlayer();
    void SavePreset();
    void LoadPreset(const std::filesystem::path& path);
    void SaveRecentPlayers() const;
    void RefreshTerrariaIntegration();
    void RefreshGameProcessState();
    void ImportSelectedTerrariaPlayer();
    void QuickStackToStorage();
    void SortInventoryById();
    void FillMoney();
    void UnlockAllResearch();
    void ApplyGodModePreset();
    void RandomizeCharacter();
    void SpawnDefinitionToFirstEmpty(int itemId, int stack);
    std::vector<std::filesystem::path> EnumeratePresets() const;
    const ItemDefinition* FindDefinition(int id) const;

    std::filesystem::path CurrentPlayerPath() const;
    std::filesystem::path RecentPlayersPath() const;
    std::filesystem::path PresetsDirectory() const;
    std::filesystem::path DetectTerrariaPlayersDirectory() const;

    std::vector<std::filesystem::path> terrariaPlayerFiles;
};
