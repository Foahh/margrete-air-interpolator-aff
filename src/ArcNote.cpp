#include "ArcNote.h"
#include <format>
#include <string>

int ArcNote::GetLength() const {
    return toT - t;
}

std::string ArcNote::ToString() const {
    return std::format("[{}, {}, {}, {}, {}, {}, {}, {}, {}, {}]", t, toT, x, toX, static_cast<int>(eX), y, toY,
                       static_cast<int>(eY), type, static_cast<int>(trace));
}

bool ArcNote::CanLinkWith(const ArcNote &other) const {
    return type == other.type && trace == other.trace && toX == other.x && toY == other.y && toT == other.t;
}
