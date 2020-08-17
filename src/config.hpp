#pragma once

#include <string>
#include <unordered_map>

#include "bitmap.hpp"

namespace rlo
{
struct RoomConfig
{
    std::string name;
    unsigned char type;
    unsigned int count;
    unsigned int minimum_size;
    float size_scaling;
    float movement_cost;
    rgb_t color;
    std::vector<std::string> attributes;
    std::unordered_map<unsigned int, float> weights;
};

std::vector<RoomConfig> read_config_from_file(const std::string &file);
}