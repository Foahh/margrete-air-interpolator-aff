#pragma once
#include <string>

#include "Common.hpp"

struct ArcNote {
    __declspec(property(get = GetLength)) int Length;

    int GetLength() const;

    int t{0};
    int toT{0};

    int x{0};
    int toX{0};
    EasingMode eX{EasingMode::Linear};

    int y{0};
    int toY{0};
    EasingMode eY{EasingMode::Linear};

    int type{0};
    bool trace{false};

    std::string ToString() const;

    bool CanLinkWith(const ArcNote &other) const;
};
