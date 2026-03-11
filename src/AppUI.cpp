#include "AppUI.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>
#include <cwchar>
#endif

namespace {
constexpr const char* kWindowOverview = "Overview###OverviewWindow";
constexpr const char* kWindowInventory = "Inventory###InventoryWindow";
constexpr const char* kWindowEquipment = "Equipment###EquipmentWindow";
constexpr const char* kWindowStorage = "Storage Vault###StorageWindow";
constexpr const char* kWindowCreator = "Creator###CreatorWindow";
constexpr const char* kWindowBuffs = "Buffs###BuffsWindow";
constexpr const char* kWindowJourney = "Journey Research###JourneyWindow";
constexpr const char* kWindowBrowser = "Item Browser###BrowserWindow";
constexpr const char* kWindowWorld = "World Flags###WorldWindow";
constexpr const char* kWindowCheats = "Cheats & Fun###CheatsWindow";
constexpr const char* kWindowSettings = "Settings###SettingsWindow";
constexpr const char* kLatestTerrariaVersion = "Desktop 1.4.5.6";
constexpr const char* kPopularMods = "Calamity, Thorium, Fargo's Souls, Spirit Mod, The Stars Above";

constexpr std::array<const char*, 4> kDifficultyNames = {"Classic", "Expert", "Master", "Journey"};
constexpr std::array<const char*, 8> kTransformationNames = {
    "None", "Bat Form", "Rat Form", "Wolf Form", "Merfolk", "Werewolf", "Shimmer Phase", "Beheaded Aura"
};

float ColorLuma(const ColorRGBA& color) {
    return (color.r * 0.2126f) + (color.g * 0.7152f) + (color.b * 0.0722f);
}

#if defined(_WIN32)
std::filesystem::path DetectRunningTerrariaExecutable() {
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return {};
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshot, &entry)) {
        CloseHandle(snapshot);
        return {};
    }

    do {
        if (_wcsicmp(entry.szExeFile, L"Terraria.exe") == 0) {
            std::filesystem::path detectedPath;
            if (HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID)) {
                std::wstring buffer(32768, L'\0');
                DWORD size = static_cast<DWORD>(buffer.size());
                if (QueryFullProcessImageNameW(process, 0, buffer.data(), &size)) {
                    buffer.resize(size);
                    detectedPath = std::filesystem::path(buffer);
                }
                CloseHandle(process);
            }

            CloseHandle(snapshot);
            return detectedPath.empty() ? std::filesystem::path(entry.szExeFile) : detectedPath;
        }
    } while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return {};
}
#endif

std::string ExtractLikelyPlayerName(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return path.stem().string();
    }

    std::string bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    std::string best = path.stem().string();
    std::string current;

    auto flushCandidate = [&]() {
        if (current.size() >= 3 && current.size() <= 24
            && current.find("Terraria") == std::string::npos
            && current.find("relogic") == std::string::npos
            && current.find('.') == std::string::npos
            && current.find('\\') == std::string::npos
            && current.find('/') == std::string::npos) {
            best = current;
        }
        current.clear();
    };

    for (unsigned char ch : bytes) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == ' ') {
            current.push_back(static_cast<char>(ch));
        } else {
            flushCandidate();
        }
    }
    flushCandidate();

    return best.empty() ? path.stem().string() : best;
}

void DrawConnectionBadge(const char* label, bool connected, const char* detail) {
    const ImVec4 color = connected ? ImVec4(0.16f, 0.55f, 0.30f, 1.0f) : ImVec4(0.55f, 0.19f, 0.20f, 1.0f);
    const ImVec4 hover = connected ? ImVec4(0.20f, 0.65f, 0.35f, 1.0f) : ImVec4(0.65f, 0.24f, 0.24f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);
    ImGui::Button(label);
    ImGui::PopStyleColor(3);

    if (detail != nullptr && detail[0] != '\0') {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", detail);
    }
}
}

AppUI::AppUI() = default;
AppUI::~AppUI() = default;

void AppUI::Init() {
    appDataDir = std::filesystem::current_path() / "data";
    std::filesystem::create_directories(appDataDir);
    std::filesystem::create_directories(PresetsDirectory());
    terrariaPlayersDirectory = DetectTerrariaPlayersDirectory();

    itemDatabase = ItemDatabase::CreateBuiltIn();
    recentPlayers = PlayerSerialization::LoadRecentPlayers(RecentPlayersPath());
    RefreshTerrariaIntegration();
    RefreshGameProcessState();

    if (std::filesystem::exists(CurrentPlayerPath())) {
        LoadCurrentPlayer();
    } else {
        SeedDemoPlayer();
        SaveCurrentPlayer();
    }

    if (presetNameBuffer[0] == '\0') {
        std::snprintf(presetNameBuffer.data(), presetNameBuffer.size(), "%s", "showcase");
    }
    if (iconPackBuffer[0] == '\0') {
        std::snprintf(iconPackBuffer.data(), iconPackBuffer.size(), "%s", "assets/icons");
    }

    ApplyTheme(activeTheme);
    SyncBuffersFromPlayer();
    statusMessage.clear();
}

void AppUI::Render() {
    if (ImGui::GetTime() >= nextIntegrationProbeTime) {
        RefreshTerrariaIntegration();
        RefreshGameProcessState();
        nextIntegrationProbeTime = ImGui::GetTime() + 2.0;
    }

    RenderDockspace();
    RenderDashboard();
    RenderInventory();
    RenderEquipment();
    RenderStorage();
    RenderCharacterCreator();
    RenderBuffsEffects();
    RenderJourneyResearch();
    RenderItemBrowser();
    RenderWorldFlags();
    RenderCheatsFun();
    RenderSettingsThemes();
}

void AppUI::RenderDockspace() {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("MainDockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(2);

    const ImGuiID dockspaceId = ImGui::GetID("TerrariaStudioDockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    BuildDefaultDockLayout(dockspaceId);
    RenderMenuBar();
    ImGui::End();
}

void AppUI::BuildDefaultDockLayout(ImGuiID dockspaceId) {
    if (layoutInitialized) {
        return;
    }

    layoutInitialized = true;
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

    ImGuiID leftId = 0;
    ImGuiID rightId = 0;
    ImGuiID bottomId = 0;
    ImGuiID centerId = dockspaceId;
    ImGuiID leftBottomId = 0;
    ImGuiID centerBottomId = 0;

    ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, 0.23f, &leftId, &centerId);
    ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, 0.27f, &rightId, &centerId);
    ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, 0.37f, &bottomId, &centerId);
    ImGui::DockBuilderSplitNode(leftId, ImGuiDir_Down, 0.40f, &leftBottomId, &leftId);
    ImGui::DockBuilderSplitNode(rightId, ImGuiDir_Down, 0.42f, &centerBottomId, &rightId);

    ImGui::DockBuilderDockWindow(kWindowInventory, centerId);
    ImGui::DockBuilderDockWindow(kWindowBrowser, bottomId);
    ImGui::DockBuilderDockWindow(kWindowJourney, leftId);
    ImGui::DockBuilderDockWindow(kWindowWorld, leftBottomId);
    ImGui::DockBuilderDockWindow(kWindowCheats, leftBottomId);
    ImGui::DockBuilderDockWindow(kWindowOverview, rightId);
    ImGui::DockBuilderDockWindow(kWindowEquipment, rightId);
    ImGui::DockBuilderDockWindow(kWindowStorage, rightId);
    ImGui::DockBuilderDockWindow(kWindowCreator, rightId);
    ImGui::DockBuilderDockWindow(kWindowBuffs, centerBottomId);
    ImGui::DockBuilderDockWindow(kWindowSettings, centerBottomId);

    ImGui::DockBuilderFinish(dockspaceId);
}

void AppUI::RenderMenuBar() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Load current player JSON")) {
            LoadCurrentPlayer();
        }
        if (ImGui::MenuItem("Save current player JSON")) {
            SaveCurrentPlayer();
        }
        if (ImGui::MenuItem("Save preset JSON")) {
            SavePreset();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Actions")) {
        if (ImGui::MenuItem("Max HP / Mana")) {
            player.maxHealth = 500;
            player.health = 500;
            player.maxMana = 200;
            player.mana = 200;
            statusMessage = "Stats maxed.";
        }
        if (ImGui::MenuItem("Fill money")) {
            FillMoney();
        }
        if (ImGui::MenuItem("Unlock all research")) {
            UnlockAllResearch();
        }
        if (ImGui::MenuItem("God mode preset")) {
            ApplyGodModePreset();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Theme")) {
        if (ImGui::MenuItem("Dark 2026", nullptr, activeTheme == ThemePreset::Dark2026)) {
            ApplyTheme(ThemePreset::Dark2026);
        }
        if (ImGui::MenuItem("Classic 1.3", nullptr, activeTheme == ThemePreset::Classic13)) {
            ApplyTheme(ThemePreset::Classic13);
        }
        if (ImGui::MenuItem("Labor of Love", nullptr, activeTheme == ThemePreset::LaborOfLove)) {
            ApplyTheme(ThemePreset::LaborOfLove);
        }
        if (ImGui::MenuItem("Bigger & Boulder Glow", nullptr, activeTheme == ThemePreset::BoulderGlow)) {
            ApplyTheme(ThemePreset::BoulderGlow);
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();
    ImGui::Text("Terraria Inventory Editor • %s", kLatestTerrariaVersion);
    ImGui::SameLine();
    ImGui::TextDisabled("by 2kae");

    if (!terrariaExecutablePath.empty()) {
        ImGui::Separator();
    } else {
        ImGui::SameLine();
        ImGui::Separator();
    }

    if (!terrariaExecutablePath.empty()) {
        DrawConnectionBadge("Terraria.exe LIVE", true, terrariaExecutablePath.string().c_str());
    } else {
        DrawConnectionBadge(!terrariaPlayersDirectory.empty() ? "Terraria folder linked" : "Terraria folder missing", !terrariaPlayersDirectory.empty(), terrariaPlayerFiles.empty() ? "No .plr found" : ".plr detected");
    }

    ImGui::EndMenuBar();
}

void AppUI::RenderDashboard() {
    ImGui::Begin(kWindowOverview);

    const bool hasSnapshot = std::filesystem::exists(CurrentPlayerPath());
    const bool hasItemDatabase = itemDatabase.size() >= 6500;
    const bool hasTerrariaFolder = !terrariaPlayersDirectory.empty();
    const bool hasTerrariaPlayers = !terrariaPlayerFiles.empty();
    const bool hasLiveProcess = !terrariaExecutablePath.empty();

    ImGui::SeparatorText("Connection status");
    DrawConnectionBadge(hasItemDatabase ? "Item DB connected" : "Item DB missing", hasItemDatabase, hasItemDatabase ? "6500 catalog entries" : "Catalog not loaded");
    DrawConnectionBadge(hasSnapshot ? "Editor snapshot ready" : "Snapshot missing", hasSnapshot, hasSnapshot ? "current_player.json loaded" : "No local snapshot");
    DrawConnectionBadge(hasTerrariaFolder ? "Terraria folder linked" : "Terraria folder missing", hasTerrariaFolder, hasTerrariaFolder ? terrariaPlayersDirectory.string().c_str() : "Documents/My Games/Terraria/Players not found");
    DrawConnectionBadge(hasTerrariaPlayers ? "Terraria players detected" : "No .plr detected", hasTerrariaPlayers, hasTerrariaPlayers ? terrariaPlayerFiles.front().filename().string().c_str() : "Place a real Terraria player save in Players");
    DrawConnectionBadge(hasLiveProcess ? "Terraria process attached" : "Terraria.exe not running", hasLiveProcess, hasLiveProcess ? terrariaExecutablePath.string().c_str() : "Launch the real game to see live process detection");
    if (hasItemDatabase && hasSnapshot && hasTerrariaFolder && hasLiveProcess) {
        ImGui::TextColored(ImVec4(0.45f, 1.0f, 0.70f, 1.0f), "Everything is connected: catalog, local snapshot, player saves folder and live Terraria process.");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.40f, 1.0f), "Not all links are live yet. The badges above show exactly what is missing.");
    }
    ImGui::Spacing();

    ImGui::Columns(2, nullptr, false);
    DrawPlayerPreview();
    ImGui::NextColumn();

    ImGui::PushItemWidth(-1.0f);
    ImGui::TextUnformatted("Name");
    if (ImGui::InputText("##Name", playerNameBuffer.data(), playerNameBuffer.size())) {
        player.name = playerNameBuffer.data();
    }
    ImGui::TextUnformatted("Difficulty");
    ImGui::Combo("##Difficulty", &player.difficulty, "Classic\0Expert\0Master\0Journey\0");
    ImGui::TextUnformatted("Hair style");
    ImGui::SliderInt("##HairStyle", &player.hairStyle, 0, 205);
    ImGui::Text("Filled slots: %d", player.filledSlotCount());
    ImGui::Text("Total stacks: %d", player.totalStackCount());
    ImGui::Text("Journey progress: %zu / %zu", player.journeyResearch.size(), itemDatabase.size());
    ImGui::TextDisabled("Developer: 2kae");
    ImGui::TextDisabled("Target version: %s", kLatestTerrariaVersion);
    ImGui::TextWrapped("Mod-friendly workflow: %s", kPopularMods);
    if (!statusMessage.empty()) {
        ImGui::TextWrapped("Last action: %s", statusMessage.c_str());
    }
    ImGui::PopItemWidth();

    if (ImGui::Button("Max HP/Mana", ImVec2(-1.0f, 0.0f))) {
        player.maxHealth = 500;
        player.health = 500;
        player.maxMana = 200;
        player.mana = 200;
        statusMessage = "Health and mana maxed.";
    }
    if (ImGui::Button("Fill Money", ImVec2(-1.0f, 0.0f))) {
        FillMoney();
    }
    if (ImGui::Button("Unlock All Research", ImVec2(-1.0f, 0.0f))) {
        UnlockAllResearch();
    }
    if (ImGui::Button("Save Snapshot", ImVec2(-1.0f, 0.0f))) {
        SaveCurrentPlayer();
    }

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::TextUnformatted("Recent players");
    if (recentPlayers.empty()) {
        ImGui::TextDisabled("No recent players yet.");
    } else {
        for (const auto& entry : recentPlayers) {
            ImGui::BulletText("%s | %s", entry.name.c_str(), entry.timestamp.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", entry.path.c_str());
        }
    }

    ImGui::SeparatorText("Terraria link");
    if (terrariaPlayersDirectory.empty()) {
        ImGui::TextDisabled("Terraria players folder was not detected yet.");
    } else {
        ImGui::TextWrapped("Players folder: %s", terrariaPlayersDirectory.string().c_str());
        if (!terrariaExecutablePath.empty()) {
            ImGui::TextWrapped("Live game process: %s", terrariaExecutablePath.string().c_str());
        }
        if (ImGui::Button("Refresh Terraria folder")) {
            RefreshTerrariaIntegration();
        }
        ImGui::SameLine();
        if (selectedTerrariaPlayerIndex >= 0 && selectedTerrariaPlayerIndex < static_cast<int>(terrariaPlayerFiles.size()) && ImGui::Button("Import selected .plr")) {
            ImportSelectedTerrariaPlayer();
        }
        ImGui::SameLine();
        if (selectedTerrariaPlayerIndex >= 0 && selectedTerrariaPlayerIndex < static_cast<int>(terrariaPlayerFiles.size()) && ImGui::Button("Use selected .plr name")) {
            player.name = terrariaPlayerFiles[static_cast<std::size_t>(selectedTerrariaPlayerIndex)].stem().string();
            SyncBuffersFromPlayer();
            statusMessage = "Player name synchronized from detected Terraria save.";
        }
        ImGui::SameLine();
        if (selectedTerrariaPlayerIndex >= 0 && selectedTerrariaPlayerIndex < static_cast<int>(terrariaPlayerFiles.size()) && ImGui::Button("Export snapshot next to .plr")) {
            const auto selectedPath = terrariaPlayerFiles[static_cast<std::size_t>(selectedTerrariaPlayerIndex)];
            const auto exportPath = selectedPath.parent_path() / (selectedPath.stem().string() + ".editor.json");
            if (PlayerSerialization::SavePlayer(player, exportPath)) {
                statusMessage = "Editor snapshot exported next to Terraria player file.";
            } else {
                statusMessage = "Failed to export snapshot next to .plr.";
            }
        }

        if (terrariaPlayerFiles.empty()) {
            ImGui::TextDisabled("No `.plr` files found in detected folder.");
        } else {
            for (std::size_t i = 0; i < terrariaPlayerFiles.size(); ++i) {
                const bool selected = static_cast<int>(i) == selectedTerrariaPlayerIndex;
                if (ImGui::Selectable(terrariaPlayerFiles[i].filename().string().c_str(), selected)) {
                    selectedTerrariaPlayerIndex = static_cast<int>(i);
                }
            }
        }
    }

    ImGui::End();
}

void AppUI::RenderInventory() {
    ImGui::Begin(kWindowInventory);
    if (ImGui::Button("Quick stack to nearby chests")) {
        QuickStackToStorage();
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto-sort by id")) {
        SortInventoryById();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Compact slots", &compactSlots);

    RenderItemGrid("InventoryGrid", player.inventory, 10, 0);
    ImGui::End();
}

void AppUI::RenderEquipment() {
    ImGui::Begin(kWindowEquipment);
    if (ImGui::BeginTabBar("EquipmentTabs")) {
        if (ImGui::BeginTabItem("Armor / Accessories")) {
            RenderItemGrid("ArmorGrid", player.armor, 5, 1000);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Dyes")) {
            RenderItemGrid("DyeGrid", player.dye, 5, 1100);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Ammo")) {
            RenderItemGrid("AmmoGrid", player.ammo, 4, 1200);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Coins")) {
            RenderItemGrid("CoinsGrid", player.coins, 4, 1300);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::Checkbox("Dead Cells vanity enabled", &player.hasDeadCellsVanity);
    ImGui::SliderInt("Ram Rune charges", &player.ramRuneCharges, 0, 10);
    ImGui::End();
}

void AppUI::RenderStorage() {
    ImGui::Begin(kWindowStorage);
    if (ImGui::BeginTabBar("StorageTabs")) {
        if (ImGui::BeginTabItem("Piggy Bank")) {
            RenderItemGrid("PiggyGrid", player.piggyBank, 10, 2000);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Safe")) {
            RenderItemGrid("SafeGrid", player.safe, 10, 2100);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Defender's Forge")) {
            RenderItemGrid("ForgeGrid", player.forge, 10, 2200);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Void Vault")) {
            RenderItemGrid("VoidVaultGrid", player.voidVault, 10, 2300);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (ImGui::Button("Fill Void Vault with Luminite set")) {
        for (std::size_t i = 0; i < player.voidVault.size(); ++i) {
            player.voidVault[i].id = 5000 + static_cast<int>(i);
            player.voidVault[i].stack = 9999;
        }
        statusMessage = "Void Vault filled with showcase loot.";
    }
    ImGui::End();
}

void AppUI::RenderCharacterCreator() {
    ImGui::Begin(kWindowCreator);
    ImGui::SliderInt("Hair Style", &player.hairStyle, 0, 205);
    ImGui::SliderInt("Skin Variant", &player.skinVariant, 0, 12);
    ImGui::ColorEdit4("Hair", &player.hairColor.r);
    ImGui::ColorEdit4("Skin", &player.skinColor.r);
    ImGui::ColorEdit4("Eyes", &player.eyeColor.r);
    ImGui::ColorEdit4("Shirt", &player.shirtColor.r);
    ImGui::ColorEdit4("Undershirt", &player.undershirtColor.r);
    ImGui::ColorEdit4("Pants", &player.pantsColor.r);
    ImGui::ColorEdit4("Shoes", &player.shoeColor.r);

    ImGui::SeparatorText("Transformations");
    ImGui::Combo("Current Transformation", &selectedTransformation, "None\0Bat Form\0Rat Form\0Wolf Form\0Merfolk\0Werewolf\0Shimmer Phase\0Beheaded Aura\0");
    if (ImGui::Button("Apply transformation")) {
        player.activeTransformations.clear();
        if (selectedTransformation > 0) {
            player.activeTransformations.push_back(selectedTransformation);
        }
        statusMessage = std::string("Transformation set to ") + kTransformationNames[static_cast<std::size_t>(selectedTransformation)];
    }
    ImGui::SameLine();
    if (ImGui::Button("Randomize look")) {
        RandomizeCharacter();
    }

    ImGui::End();
}

void AppUI::RenderBuffsEffects() {
    ImGui::Begin(kWindowBuffs);
    ImGui::InputInt("Buff ID", &selectedBuffId);
    ImGui::InputFloat("Duration (sec)", &selectedBuffTime, 30.0f, 300.0f, "%.0f");
    if (ImGui::Button("Add / update buff")) {
        auto it = std::find_if(player.buffs.begin(), player.buffs.end(), [this](const auto& buff) {
            return buff.first == selectedBuffId;
        });
        if (it == player.buffs.end()) {
            player.buffs.emplace_back(selectedBuffId, selectedBuffTime);
        } else {
            it->second = selectedBuffTime;
        }
        statusMessage = "Buff applied.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Infinite buffs")) {
        for (auto& buff : player.buffs) {
            buff.second = 36000.0f;
        }
        statusMessage = "All buffs set to near-infinite duration.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear debuffs / buffs")) {
        player.buffs.clear();
        statusMessage = "Buff list cleared.";
    }

    if (ImGui::BeginTable("BuffTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Time");
        ImGui::TableHeadersRow();
        for (std::size_t i = 0; i < player.buffs.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", player.buffs[i].first);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Buff %d", player.buffs[i].first);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.0f sec", player.buffs[i].second);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void AppUI::RenderJourneyResearch() {
    ImGui::Begin(kWindowJourney);
    const float progress = itemDatabase.empty() ? 0.0f : static_cast<float>(player.journeyResearch.size()) / static_cast<float>(itemDatabase.size());
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
    ImGui::Text("%zu / %zu researched", player.journeyResearch.size(), itemDatabase.size());

    if (ImGui::Button("Research max for selected item")) {
        player.journeyResearch[selectedDefinitionId] = 9999;
        statusMessage = "Selected item fully researched.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Unlock 100%")) {
        UnlockAllResearch();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset research")) {
        player.journeyResearch.clear();
        statusMessage = "Journey research reset.";
    }

    const auto filtered = ItemDatabase::Filter(itemDatabase, searchBuffer.data(), filterNew145, filterWhips, filterDeadCells, filterBoulders, 250);
    if (ImGui::BeginTable("JourneyTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 320.0f))) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Tags", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Research", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableHeadersRow();

        for (const ItemDefinition* def : filtered) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", def->id);
            ImGui::TableSetColumnIndex(1);
            const bool selected = selectedDefinitionId == def->id;
            if (ImGui::Selectable(def->name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                selectedDefinitionId = def->id;
            }
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(def->category.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(def->tag.c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%d", player.journeyResearch.contains(def->id) ? player.journeyResearch[def->id] : 0);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void AppUI::RenderItemBrowser() {
    ImGui::Begin(kWindowBrowser);
    ImGui::InputText("Search by name / id / tag", searchBuffer.data(), searchBuffer.size());
    ImGui::SliderInt("Spawn stack", &browserSpawnStack, 1, 9999);
    ImGui::Checkbox("New in 1.4.5", &filterNew145);
    ImGui::SameLine();
    ImGui::Checkbox("Whips", &filterWhips);
    ImGui::SameLine();
    ImGui::Checkbox("Dead Cells", &filterDeadCells);
    ImGui::SameLine();
    ImGui::Checkbox("Boulders", &filterBoulders);

    const auto filtered = ItemDatabase::Filter(itemDatabase, searchBuffer.data(), filterNew145, filterWhips, filterDeadCells, filterBoulders, 300);
    if (const ItemDefinition* selected = FindDefinition(selectedDefinitionId)) {
        ImGui::SeparatorText("Selected item");
        ImGui::Text("#%d %s", selected->id, selected->name.c_str());
        ImGui::Text("Category: %s | Tag: %s | Max stack: %d", selected->category.c_str(), selected->tag.c_str(), selected->maxStack);
        if (ImGui::Button("Add to first empty slot")) {
            SpawnDefinitionToFirstEmpty(selected->id, std::min(browserSpawnStack, selected->maxStack));
        }
        ImGui::SameLine();
        if (ImGui::Button("Add 999 to first empty slot")) {
            SpawnDefinitionToFirstEmpty(selected->id, std::min(999, selected->maxStack));
        }
    }

    if (ImGui::BeginTable("ItemBrowserTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 380.0f))) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableHeadersRow();

        for (const ItemDefinition* def : filtered) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", def->id);
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Selectable(def->name.c_str(), selectedDefinitionId == def->id, ImGuiSelectableFlags_SpanAllColumns)) {
                selectedDefinitionId = def->id;
            }
            if (ImGui::BeginDragDropSource()) {
                SpawnPayload payload{def->id, std::min(browserSpawnStack, def->maxStack)};
                ImGui::SetDragDropPayload("ITEM_SPAWN", &payload, sizeof(payload));
                ImGui::Text("Spawn %s", def->name.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(def->category.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(def->tag.c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s%s%s%s",
                def->isNew145 ? "1.4.5 " : "",
                def->isWhip ? "Whip " : "",
                def->isDeadCells ? "DC " : "",
                def->isBoulderRelated ? "Boulder" : "");
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void AppUI::RenderWorldFlags() {
    ImGui::Begin(kWindowWorld);
    ImGui::Checkbox("Skyblock start", &player.isSkyblockStart);
    ImGui::InputInt("Spawn X", &player.spawnX);
    ImGui::InputInt("Spawn Y", &player.spawnY);
    static bool floatingIslandStart = true;
    static bool rainbowBoulders = true;
    static bool lavaBoulders = true;
    static bool secretSeedCelebration = false;

    ImGui::Checkbox("Floating island start", &floatingIslandStart);
    ImGui::Checkbox("Rainbow boulders enabled", &rainbowBoulders);
    ImGui::Checkbox("Lava boulders enabled", &lavaBoulders);
    ImGui::Checkbox("Secret seed preset: Celebration", &secretSeedCelebration);

    if (ImGui::Button("Apply skyblock showcase preset")) {
        player.isSkyblockStart = true;
        player.spawnX = 420;
        player.spawnY = 128;
        statusMessage = "Skyblock preset applied.";
    }
    ImGui::End();
}

void AppUI::RenderCheatsFun() {
    ImGui::Begin(kWindowCheats);
    if (ImGui::Button("God mode preset", ImVec2(-1.0f, 0.0f))) {
        ApplyGodModePreset();
    }
    if (ImGui::Button("9999 platinum", ImVec2(-1.0f, 0.0f))) {
        FillMoney();
    }
    if (ImGui::Button("Fill Void Vault with 1.4.5 items", ImVec2(-1.0f, 0.0f))) {
        for (std::size_t i = 0; i < player.voidVault.size(); ++i) {
            const int id = 5851 + static_cast<int>(i);
            player.voidVault[i].id = id;
            player.voidVault[i].stack = 9999;
        }
        statusMessage = "Void Vault filled with late-game showcase items.";
    }
    if (ImGui::Button("Randomize character for streamer mode", ImVec2(-1.0f, 0.0f))) {
        RandomizeCharacter();
    }
    if (ImGui::Button("Clear entire inventory", ImVec2(-1.0f, 0.0f))) {
        for (auto& item : player.inventory) {
            item.clear();
        }
        statusMessage = "Inventory cleared.";
    }
    ImGui::End();
}

void AppUI::RenderSettingsThemes() {
    ImGui::Begin(kWindowSettings);
    ImGui::SliderFloat("UI scale", &uiScale, 0.8f, 1.8f, "%.2f");
    if (std::abs(uiScale - appliedUiScale) > 0.001f) {
        ImGui::GetStyle().ScaleAllSizes(uiScale / appliedUiScale);
        appliedUiScale = uiScale;
    }

    if (ImGui::Button("Dark 2026")) {
        ApplyTheme(ThemePreset::Dark2026);
    }
    ImGui::SameLine();
    if (ImGui::Button("Classic 1.3")) {
        ApplyTheme(ThemePreset::Classic13);
    }
    ImGui::SameLine();
    if (ImGui::Button("Labor of Love")) {
        ApplyTheme(ThemePreset::LaborOfLove);
    }
    ImGui::SameLine();
    if (ImGui::Button("Boulder Glow")) {
        ApplyTheme(ThemePreset::BoulderGlow);
    }

    ImGui::SeparatorText("Presets");
    ImGui::InputText("Preset name", presetNameBuffer.data(), presetNameBuffer.size());
    if (ImGui::Button("Save preset")) {
        SavePreset();
    }

    for (const auto& preset : EnumeratePresets()) {
        ImGui::PushID(preset.string().c_str());
        ImGui::TextUnformatted(preset.filename().string().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            LoadPreset(preset);
        }
        ImGui::PopID();
    }

    ImGui::SeparatorText("Icons pack");
    ImGui::InputText("PNG folder", iconPackBuffer.data(), iconPackBuffer.size());
    if (ImGui::Button("Use folder path")) {
        statusMessage = std::string("Icons pack path set to ") + iconPackBuffer.data() + ". Runtime PNG preview hook prepared.";
    }
    ImGui::TextDisabled("This build is self-contained and keeps icon loading optional.");

    ImGui::End();
}

void AppUI::RenderItemGrid(const char* id, std::vector<Item>& items, int columns, int idBase) {
    const float slotWidth = compactSlots ? 52.0f : 68.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float availableWidth = std::max(ImGui::GetContentRegionAvail().x, slotWidth);
    const int autoColumns = std::max(1, static_cast<int>((availableWidth + spacing) / (slotWidth + spacing)));
    const int effectiveColumns = std::max(1, std::min(columns, autoColumns));

    if (ImGui::BeginTable(id, effectiveColumns, ImGuiTableFlags_SizingFixedFit)) {
        for (std::size_t i = 0; i < items.size(); ++i) {
            ImGui::TableNextColumn();
            DrawItemSlot(items[i], idBase + static_cast<int>(i), id);
        }
        ImGui::EndTable();
    }
}

void AppUI::DrawItemSlot(Item& item, int index, const char* labelPrefix) {
    ImGui::PushID(index);
    const ImVec2 size = compactSlots ? ImVec2(52.0f, 52.0f) : ImVec2(68.0f, 68.0f);
    const ItemDefinition* definition = FindDefinition(item.id);

    ImVec4 baseColor = item.isEmpty() ? ImVec4(0.18f, 0.2f, 0.24f, 1.0f) : ImVec4(0.22f, 0.31f, 0.38f, 1.0f);
    if (item.favorite) {
        baseColor = ImVec4(0.52f, 0.35f, 0.12f, 1.0f);
    }
    if (item.locked) {
        baseColor = ImVec4(0.42f, 0.15f, 0.15f, 1.0f);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(baseColor.x + 0.08f, baseColor.y + 0.08f, baseColor.z + 0.08f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(baseColor.x + 0.12f, baseColor.y + 0.12f, baseColor.z + 0.12f, 1.0f));
    ImGui::Button("##slot", size);
    ImGui::PopStyleColor(3);

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(ImVec2(min.x + 2.0f, min.y + 2.0f), max, IM_COL32(0, 0, 0, 28), 8.0f);
    drawList->AddRectFilledMultiColor(min, max, IM_COL32(255, 255, 255, 10), IM_COL32(255, 255, 255, 4), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 12));
    drawList->AddRect(min, max, IM_COL32(255, 255, 255, 50), 6.0f, 0, 1.0f);

    const std::string title = item.isEmpty() ? "+" : std::to_string(item.id);
    drawList->AddText(ImVec2(min.x + 6.0f, min.y + 5.0f), IM_COL32(255, 255, 255, 220), title.c_str());
    if (!item.isEmpty()) {
        const std::string stackText = std::to_string(item.stack);
        drawList->AddText(ImVec2(min.x + 6.0f, max.y - 20.0f), IM_COL32(255, 240, 180, 255), stackText.c_str());
        if (definition) {
            const std::size_t maxChars = compactSlots ? 7 : 10;
            std::string shortName = definition->name.substr(0, std::min<std::size_t>(definition->name.size(), maxChars));
            if (definition->name.size() > maxChars) {
                shortName += compactSlots ? "" : "…";
            }
            drawList->AddText(ImVec2(min.x + 6.0f, min.y + 24.0f), IM_COL32(180, 220, 255, 255), shortName.c_str());
        }
    }

    if (ImGui::BeginDragDropSource()) {
        SlotPayload payload{&item};
        ImGui::SetDragDropPayload("ITEM_SLOT", &payload, sizeof(payload));
        ImGui::TextUnformatted(item.isEmpty() ? "Empty slot" : item.debugLabel().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (!item.locked) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ITEM_SLOT")) {
                const auto slotPayload = *static_cast<const SlotPayload*>(payload->Data);
                if (slotPayload.source != nullptr && slotPayload.source != &item && !slotPayload.source->locked) {
                    std::swap(*slotPayload.source, item);
                    statusMessage = "Items swapped.";
                }
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ITEM_SPAWN")) {
                const auto spawnPayload = *static_cast<const SpawnPayload*>(payload->Data);
                item.id = spawnPayload.itemId;
                item.stack = spawnPayload.stack;
                item.prefix = 0;
                statusMessage = "Item spawned into slot.";
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (definition) {
            ImGui::Text("%s", definition->name.c_str());
            ImGui::TextDisabled("%s | %s", definition->category.c_str(), definition->tag.c_str());
        } else {
            ImGui::TextUnformatted(item.isEmpty() ? "Empty slot" : "Unknown item");
        }
        ImGui::Text("ID: %d", item.id);
        ImGui::Text("Stack: %d", item.stack);
        ImGui::Text("Prefix: %u", item.prefix);
        ImGui::Text("Favorite: %s", item.favorite ? "Yes" : "No");
        ImGui::Text("Locked: %s", item.locked ? "Yes" : "No");
        ImGui::EndTooltip();
    }

    if (ImGui::BeginPopupContextItem("SlotContext")) {
        ImGui::Checkbox("Favorite", &item.favorite);
        ImGui::Checkbox("Locked", &item.locked);
        ImGui::InputInt("ID", &item.id);
        item.id = std::max(item.id, 0);
        ImGui::InputInt("Stack", &item.stack);
        item.stack = std::clamp(item.stack, 1, 9999);
        int prefix = item.prefix;
        ImGui::SliderInt("Prefix", &prefix, 0, 255);
        item.prefix = static_cast<std::uint8_t>(prefix);
        if (ImGui::Button("Clear slot")) {
            item.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

void AppUI::DrawPlayerPreview() {
    ImGui::BeginChild("PlayerPreview", ImVec2(0.0f, 260.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::TextUnformatted("Character preview");

    const ImVec2 canvas = ImGui::GetContentRegionAvail();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, ImVec2(origin.x + canvas.x, origin.y + canvas.y), IM_COL32(17, 26, 39, 255), 12.0f);

    const ImVec2 center(origin.x + canvas.x * 0.5f, origin.y + canvas.y * 0.5f + 18.0f);
    const float scale = std::min(canvas.x, canvas.y) * 0.18f;

    drawList->AddCircleFilled(ImVec2(center.x, center.y - scale * 2.5f), scale * 0.7f, ImGui::ColorConvertFloat4ToU32(ImVec4(player.skinColor.r, player.skinColor.g, player.skinColor.b, 1.0f)));
    drawList->AddRectFilled(ImVec2(center.x - scale * 0.8f, center.y - scale * 1.6f), ImVec2(center.x + scale * 0.8f, center.y + scale * 0.2f), ImGui::ColorConvertFloat4ToU32(ImVec4(player.shirtColor.r, player.shirtColor.g, player.shirtColor.b, 1.0f)), 8.0f);
    drawList->AddRectFilled(ImVec2(center.x - scale * 0.7f, center.y + scale * 0.2f), ImVec2(center.x + scale * 0.7f, center.y + scale * 1.8f), ImGui::ColorConvertFloat4ToU32(ImVec4(player.pantsColor.r, player.pantsColor.g, player.pantsColor.b, 1.0f)), 8.0f);
    drawList->AddRectFilled(ImVec2(center.x - scale * 1.3f, center.y - scale * 1.3f), ImVec2(center.x - scale * 0.8f, center.y + scale * 0.5f), ImGui::ColorConvertFloat4ToU32(ImVec4(player.skinColor.r, player.skinColor.g, player.skinColor.b, 1.0f)), 6.0f);
    drawList->AddRectFilled(ImVec2(center.x + scale * 0.8f, center.y - scale * 1.3f), ImVec2(center.x + scale * 1.3f, center.y + scale * 0.5f), ImGui::ColorConvertFloat4ToU32(ImVec4(player.skinColor.r, player.skinColor.g, player.skinColor.b, 1.0f)), 6.0f);
    drawList->AddRectFilled(ImVec2(center.x - scale * 0.65f, center.y + scale * 1.8f), ImVec2(center.x - scale * 0.15f, center.y + scale * 3.1f), ImGui::ColorConvertFloat4ToU32(ImVec4(player.skinColor.r, player.skinColor.g, player.skinColor.b, 1.0f)), 6.0f);
    drawList->AddRectFilled(ImVec2(center.x + scale * 0.15f, center.y + scale * 1.8f), ImVec2(center.x + scale * 0.65f, center.y + scale * 3.1f), ImGui::ColorConvertFloat4ToU32(ImVec4(player.skinColor.r, player.skinColor.g, player.skinColor.b, 1.0f)), 6.0f);
    drawList->AddRectFilled(ImVec2(center.x - scale * 0.85f, center.y + scale * 3.0f), ImVec2(center.x - scale * 0.05f, center.y + scale * 3.4f), ImGui::ColorConvertFloat4ToU32(ImVec4(player.shoeColor.r, player.shoeColor.g, player.shoeColor.b, 1.0f)), 6.0f);
    drawList->AddRectFilled(ImVec2(center.x + scale * 0.05f, center.y + scale * 3.0f), ImVec2(center.x + scale * 0.85f, center.y + scale * 3.4f), ImGui::ColorConvertFloat4ToU32(ImVec4(player.shoeColor.r, player.shoeColor.g, player.shoeColor.b, 1.0f)), 6.0f);
    drawList->AddCircleFilled(ImVec2(center.x, center.y - scale * 2.8f), scale * 0.75f, ImGui::ColorConvertFloat4ToU32(ImVec4(player.hairColor.r, player.hairColor.g, player.hairColor.b, 0.9f)), 20);

    if (player.hasDeadCellsVanity) {
        drawList->AddText(ImVec2(origin.x + 16.0f, origin.y + 16.0f), IM_COL32(255, 166, 0, 255), "Beheaded Vanity Equipped");
    }
    if (!player.activeTransformations.empty()) {
        drawList->AddText(ImVec2(origin.x + 16.0f, origin.y + 36.0f), IM_COL32(120, 255, 180, 255), kTransformationNames[static_cast<std::size_t>(std::clamp(player.activeTransformations.front(), 0, static_cast<int>(kTransformationNames.size() - 1)))]);
    }

    const ImU32 captionColor = ColorLuma(player.hairColor) > 0.5f ? IM_COL32(20, 20, 25, 255) : IM_COL32(240, 240, 250, 255);
    drawList->AddText(ImVec2(origin.x + canvas.x - 190.0f, origin.y + 18.0f), captionColor, player.name.c_str());
    ImGui::Dummy(canvas);
    ImGui::EndChild();
}

void AppUI::SeedDemoPlayer() {
    player = PlayerData{};
    player.name = "Terrarian 2026";
    player.difficulty = 3;
    player.hairStyle = 144;
    player.skinVariant = 2;
    player.health = 500;
    player.maxHealth = 500;
    player.mana = 200;
    player.maxMana = 200;
    player.isSkyblockStart = true;
    player.spawnX = 420;
    player.spawnY = 128;
    player.hasDeadCellsVanity = true;
    player.ramRuneCharges = 3;
    player.activeTransformations = {1};
    player.buffs = {{26, 900.0f}, {101, 1200.0f}, {222, 3600.0f}};

    const std::array<int, 10> hotbarIds = {6, 8, 9, 10, 17, 12, 13, 14, 4, 5};
    for (std::size_t i = 0; i < hotbarIds.size(); ++i) {
        player.inventory[i].id = hotbarIds[i];
        player.inventory[i].stack = (i < 6) ? 1 : 999;
        player.inventory[i].favorite = i < 3;
    }

    player.armor[0] = Item{15, 1, 0, true, false, 0};
    player.armor[1] = Item{16, 1, 0, true, false, 0};
    player.armor[2] = Item{17, 1, 0, true, false, 0};
    player.coins[0] = Item{71, 9999, 0, false, false, 0};
    player.coins[1] = Item{72, 9999, 0, false, false, 0};
    player.coins[2] = Item{73, 9999, 0, false, false, 0};
    player.coins[3] = Item{74, 9999, 0, false, false, 0};

    for (int i = 0; i < 18; ++i) {
        player.journeyResearch[itemDatabase.empty() ? i + 1 : itemDatabase[static_cast<std::size_t>(i)].id] = 9999;
    }
    player.ensureSizes();
}

void AppUI::SyncBuffersFromPlayer() {
    std::fill(playerNameBuffer.begin(), playerNameBuffer.end(), '\0');
    std::snprintf(playerNameBuffer.data(), playerNameBuffer.size(), "%s", player.name.c_str());
}

void AppUI::ApplyTheme(ThemePreset preset) {
    activeTheme = preset;
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.CellPadding = ImVec2(5.0f, 5.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 5.0f);
    style.WindowRounding = 12.0f;
    style.ChildRounding = 10.0f;
    style.FrameRounding = 10.0f;
    style.GrabRounding = 10.0f;
    style.TabRounding = 10.0f;
    style.ScrollbarRounding = 10.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.SeparatorTextBorderSize = 1.0f;
    style.SeparatorTextAlign = ImVec2(0.02f, 0.5f);

    auto& colors = style.Colors;
    if (preset == ThemePreset::Dark2026) {
        ImGui::StyleColorsDark();
        colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.07f, 0.10f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.09f, 0.13f, 0.95f);
        colors[ImGuiCol_Border] = ImVec4(0.20f, 0.31f, 0.40f, 0.55f);
        colors[ImGuiCol_Text] = ImVec4(0.92f, 0.96f, 1.0f, 1.0f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.62f, 0.72f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.45f, 0.65f, 0.95f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.54f, 0.78f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(0.16f, 0.32f, 0.42f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.40f, 0.55f, 1.0f);
        colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.18f, 0.23f, 1.0f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.36f, 0.48f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.12f, 0.18f, 1.0f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.19f, 0.31f, 0.95f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.24f, 0.38f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.29f, 0.44f, 1.0f);
    } else if (preset == ThemePreset::Classic13) {
        ImGui::StyleColorsDark();
        colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.11f, 0.16f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.13f, 0.18f, 0.95f);
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.24f, 0.16f, 0.60f);
        colors[ImGuiCol_Button] = ImVec4(0.30f, 0.26f, 0.16f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.32f, 0.20f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.36f, 0.28f, 0.17f, 1.0f);
        colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.14f, 0.09f, 1.0f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.27f, 0.12f, 1.0f);
    } else if (preset == ThemePreset::LaborOfLove) {
        ImGui::StyleColorsDark();
        colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.06f, 0.11f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.08f, 0.14f, 0.95f);
        colors[ImGuiCol_Border] = ImVec4(0.42f, 0.20f, 0.31f, 0.55f);
        colors[ImGuiCol_Button] = ImVec4(0.62f, 0.22f, 0.42f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.72f, 0.28f, 0.50f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.55f, 0.17f, 0.36f, 1.0f);
        colors[ImGuiCol_Tab] = ImVec4(0.23f, 0.10f, 0.17f, 1.0f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.62f, 0.22f, 0.42f, 1.0f);
    } else {
        ImGui::StyleColorsDark();
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.04f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.06f, 0.95f);
        colors[ImGuiCol_Border] = ImVec4(0.35f, 0.42f, 0.10f, 0.55f);
        colors[ImGuiCol_Button] = ImVec4(0.42f, 0.46f, 0.08f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.56f, 0.62f, 0.12f, 1.0f);
        colors[ImGuiCol_Header] = ImVec4(0.49f, 0.58f, 0.10f, 1.0f);
        colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.19f, 0.06f, 1.0f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.49f, 0.58f, 0.10f, 1.0f);
    }

    statusMessage = "Theme applied.";
}

void AppUI::SaveCurrentPlayer() {
    if (PlayerSerialization::SavePlayer(player, CurrentPlayerPath())) {
        PlayerSerialization::TouchRecentPlayer(recentPlayers, player.name, CurrentPlayerPath());
        SaveRecentPlayers();
        statusMessage = "Current player saved to JSON.";
    } else {
        statusMessage = "Failed to save current player JSON.";
    }
}

void AppUI::LoadCurrentPlayer() {
    if (PlayerSerialization::LoadPlayer(player, CurrentPlayerPath())) {
        SyncBuffersFromPlayer();
        PlayerSerialization::TouchRecentPlayer(recentPlayers, player.name, CurrentPlayerPath());
        SaveRecentPlayers();
        statusMessage = "Current player loaded from JSON.";
    } else {
        statusMessage = "Failed to load current player JSON.";
    }
}

void AppUI::SavePreset() {
    std::string name = presetNameBuffer.data();
    if (name.empty()) {
        name = "preset";
    }
    const auto path = PresetsDirectory() / (name + ".json");
    if (PlayerSerialization::SavePlayer(player, path)) {
        statusMessage = "Preset saved: " + path.filename().string();
    } else {
        statusMessage = "Failed to save preset.";
    }
}

void AppUI::LoadPreset(const std::filesystem::path& path) {
    if (PlayerSerialization::LoadPlayer(player, path)) {
        SyncBuffersFromPlayer();
        statusMessage = "Preset loaded: " + path.filename().string();
    } else {
        statusMessage = "Failed to load preset.";
    }
}

void AppUI::SaveRecentPlayers() const {
    PlayerSerialization::SaveRecentPlayers(recentPlayers, RecentPlayersPath());
}

void AppUI::RefreshTerrariaIntegration() {
    const auto previousFiles = terrariaPlayerFiles;
    terrariaPlayerFiles.clear();
    terrariaPlayersDirectory = DetectTerrariaPlayersDirectory();

    if (terrariaPlayersDirectory.empty() || !std::filesystem::exists(terrariaPlayersDirectory)) {
        selectedTerrariaPlayerIndex = -1;
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(terrariaPlayersDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".plr") {
            terrariaPlayerFiles.push_back(entry.path());
        }
    }

    std::sort(terrariaPlayerFiles.begin(), terrariaPlayerFiles.end());
    if (!terrariaPlayerFiles.empty()) {
        selectedTerrariaPlayerIndex = std::clamp(selectedTerrariaPlayerIndex, 0, static_cast<int>(terrariaPlayerFiles.size()) - 1);
    } else {
        selectedTerrariaPlayerIndex = -1;
    }

    if (previousFiles != terrariaPlayerFiles) {
        statusMessage = terrariaPlayerFiles.empty()
            ? "Terraria players folder updated: no .plr files detected."
            : ("Terraria players folder updated: " + std::to_string(terrariaPlayerFiles.size()) + " .plr file(s) detected.");
    }
}

void AppUI::RefreshGameProcessState() {
#if defined(_WIN32)
    terrariaExecutablePath = DetectRunningTerrariaExecutable();
#else
    terrariaExecutablePath.clear();
#endif
}

void AppUI::ImportSelectedTerrariaPlayer() {
    if (selectedTerrariaPlayerIndex < 0 || selectedTerrariaPlayerIndex >= static_cast<int>(terrariaPlayerFiles.size())) {
        statusMessage = "No Terraria player file selected.";
        return;
    }

    const auto selectedPath = terrariaPlayerFiles[static_cast<std::size_t>(selectedTerrariaPlayerIndex)];
    const auto sidecarPath = selectedPath.parent_path() / (selectedPath.stem().string() + ".editor.json");

    if (std::filesystem::exists(sidecarPath) && PlayerSerialization::LoadPlayer(player, sidecarPath)) {
        SyncBuffersFromPlayer();
        statusMessage = "Imported full editor data from sidecar JSON next to selected .plr.";
        return;
    }

    player.ensureSizes();
    player.name = ExtractLikelyPlayerName(selectedPath);
    SyncBuffersFromPlayer();

    std::error_code ec;
    const auto fileSize = std::filesystem::file_size(selectedPath, ec);
    statusMessage = ec
        ? "Imported basic metadata from selected .plr."
        : ("Imported basic metadata from selected .plr (" + std::to_string(fileSize) + " bytes).");
}

void AppUI::QuickStackToStorage() {
    auto tryMerge = [](std::vector<Item>& destination, Item& source) {
        if (source.isEmpty()) {
            return false;
        }
        for (auto& target : destination) {
            if (target.id == source.id && !target.isEmpty() && target.stack < 9999) {
                const int transferable = std::min(9999 - target.stack, source.stack);
                target.stack += transferable;
                source.stack -= transferable;
                if (source.stack <= 0) {
                    source.clear();
                    return true;
                }
            }
        }
        for (auto& target : destination) {
            if (target.isEmpty()) {
                target = source;
                source.clear();
                return true;
            }
        }
        return false;
    };

    int moved = 0;
    for (auto& source : player.inventory) {
        if (tryMerge(player.piggyBank, source) || tryMerge(player.safe, source) || tryMerge(player.forge, source) || tryMerge(player.voidVault, source)) {
            ++moved;
        }
    }
    statusMessage = "Quick stack moved " + std::to_string(moved) + " inventory stacks.";
}

void AppUI::SortInventoryById() {
    std::stable_sort(player.inventory.begin(), player.inventory.end(), [](const Item& lhs, const Item& rhs) {
        if (lhs.isEmpty() != rhs.isEmpty()) {
            return !lhs.isEmpty();
        }
        if (lhs.favorite != rhs.favorite) {
            return lhs.favorite;
        }
        return lhs.id < rhs.id;
    });
    statusMessage = "Inventory auto-sorted by id and favorites.";
}

void AppUI::FillMoney() {
    player.coins[0] = Item{71, 9999, 0, false, false, 0};
    player.coins[1] = Item{72, 9999, 0, false, false, 0};
    player.coins[2] = Item{73, 9999, 0, false, false, 0};
    player.coins[3] = Item{74, 9999, 0, false, false, 0};
    statusMessage = "Money filled to 9999 platinum / gold / silver / copper.";
}

void AppUI::UnlockAllResearch() {
    for (const auto& definition : itemDatabase) {
        player.journeyResearch[definition.id] = definition.maxStack;
    }
    statusMessage = "Journey research unlocked for entire database.";
}

void AppUI::ApplyGodModePreset() {
    player.maxHealth = 500;
    player.health = 500;
    player.maxMana = 200;
    player.mana = 200;
    player.hasDeadCellsVanity = true;
    player.ramRuneCharges = 5;
    player.buffs = {{26, 36000.0f}, {49, 36000.0f}, {114, 36000.0f}, {222, 36000.0f}};
    player.activeTransformations = {7};

    const std::array<int, 10> showcase = {6, 8, 9, 10, 11, 15, 16, 17, 18, 12};
    for (std::size_t i = 0; i < showcase.size() && i < player.inventory.size(); ++i) {
        player.inventory[i].id = showcase[i];
        player.inventory[i].stack = 1;
        player.inventory[i].favorite = true;
    }
    FillMoney();
    UnlockAllResearch();
    statusMessage = "God mode preset applied: Zenith, whips, Dead Cells and infinite research.";
}

void AppUI::RandomizeCharacter() {
    std::mt19937 rng(static_cast<std::mt19937::result_type>(std::random_device{}()));
    std::uniform_int_distribution<int> hairDistribution(0, 205);
    std::uniform_real_distribution<float> colorDistribution(0.0f, 1.0f);

    player.hairStyle = hairDistribution(rng);
    player.skinVariant = hairDistribution(rng) % 13;
    auto randomizeColor = [&]() {
        return ColorRGBA{colorDistribution(rng), colorDistribution(rng), colorDistribution(rng), 1.0f};
    };
    player.hairColor = randomizeColor();
    player.skinColor = randomizeColor();
    player.eyeColor = randomizeColor();
    player.shirtColor = randomizeColor();
    player.undershirtColor = randomizeColor();
    player.pantsColor = randomizeColor();
    player.shoeColor = randomizeColor();
    statusMessage = "Character randomized.";
}

void AppUI::SpawnDefinitionToFirstEmpty(int itemId, int stack) {
    for (auto& item : player.inventory) {
        if (item.isEmpty()) {
            item.id = itemId;
            item.stack = stack;
            item.prefix = 0;
            statusMessage = "Spawned item into first empty inventory slot.";
            return;
        }
    }
    statusMessage = "No free inventory slot found.";
}

std::vector<std::filesystem::path> AppUI::EnumeratePresets() const {
    std::vector<std::filesystem::path> presets;
    if (!std::filesystem::exists(PresetsDirectory())) {
        return presets;
    }

    for (const auto& entry : std::filesystem::directory_iterator(PresetsDirectory())) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            presets.push_back(entry.path());
        }
    }
    std::sort(presets.begin(), presets.end());
    return presets;
}

const ItemDefinition* AppUI::FindDefinition(int id) const {
    return ItemDatabase::FindById(itemDatabase, id);
}

std::filesystem::path AppUI::CurrentPlayerPath() const {
    return appDataDir / "current_player.json";
}

std::filesystem::path AppUI::RecentPlayersPath() const {
    return appDataDir / "recent_players.json";
}

std::filesystem::path AppUI::PresetsDirectory() const {
    return appDataDir / "presets";
}

std::filesystem::path AppUI::DetectTerrariaPlayersDirectory() const {
#if defined(_WIN32)
    if (const char* userProfile = std::getenv("USERPROFILE")) {
        const auto candidate = std::filesystem::path(userProfile) / "Documents" / "My Games" / "Terraria" / "Players";
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
#endif

    if (const char* home = std::getenv("HOME")) {
        const std::array<std::filesystem::path, 3> candidates = {
            std::filesystem::path(home) / "Documents" / "My Games" / "Terraria" / "Players",
            std::filesystem::path(home) / ".local" / "share" / "Terraria" / "Players",
            std::filesystem::path(home) / "Library" / "Application Support" / "Terraria" / "Players"
        };

        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate)) {
                return candidate;
            }
        }
    }

    return {};
}
