#pragma once

#include <vector>

#include "config.hpp"
#include "map.hpp"

namespace rlo
{
float evaluate(const Map &map, const std::vector<RoomConfig> &config);
}