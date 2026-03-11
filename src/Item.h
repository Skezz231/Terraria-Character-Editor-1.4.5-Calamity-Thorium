#pragma once
#include <cstdint>
#include <string>

struct Item {
    int32_t id          = 0;
    int32_t stack       = 1;
    uint8_t prefix      = 0;
    bool    favorite    = false;
    bool    locked      = false;          // 1.4.5 locks
    int     modDataSize = 0;              // For mods
    
    // UI Helpers
    bool isEmpty() const { return id == 0 || stack <= 0; }
    void clear() {
        id = 0;
        stack = 1;
        prefix = 0;
        favorite = false;
        locked = false;
        modDataSize = 0;
    }

    std::string debugLabel() const {
        return isEmpty() ? "Empty" : ("ID " + std::to_string(id) + " x" + std::to_string(stack));
    }
};
