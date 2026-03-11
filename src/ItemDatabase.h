#pragma once
#include <string>
#include <vector>

struct ItemDefinition {
    int id = 0;
    std::string name;
    std::string category;
    std::string tag;
    int maxStack = 9999;
    int value = 0;
    bool isNew145 = false;
    bool isWhip = false;
    bool isDeadCells = false;
    bool isBoulderRelated = false;
};

class ItemDatabase {
public:
    static std::vector<ItemDefinition> CreateBuiltIn();
    static const ItemDefinition* FindById(const std::vector<ItemDefinition>& database, int id);
    static std::vector<const ItemDefinition*> Filter(
        const std::vector<ItemDefinition>& database,
        const std::string& search,
        bool onlyNew145,
        bool onlyWhips,
        bool onlyDeadCells,
        bool onlyBoulders,
        std::size_t limit);
};
