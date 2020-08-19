#include <random>

#include <doctest/doctest.h>

#include "optimize.hpp"
#include "config.hpp"
#include "map.hpp"

namespace rlo
{
constexpr unsigned int map_size = 100;
constexpr unsigned int minimum_room_size = 4;
constexpr unsigned int maximum_room_size = 20;

std::vector<Room> generate_random_rooms(const std::vector<RoomConfig> &config)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<unsigned int> position_dist(0, map_size);
    std::uniform_int_distribution<unsigned int> size_dist(minimum_room_size, maximum_room_size);

    std::vector<Room> rooms;
    for (const auto &room_config : config)
    {
        for (int i = 0; i < room_config.count; i++)
        {
            const unsigned int width = size_dist(rng);
            const unsigned int height = size_dist(rng);
            std::uniform_int_distribution<unsigned int> door_x_dist(0, width);
            std::uniform_int_distribution<unsigned int> door_y_dist(0, height);

            rooms.emplace_back(
                Room{room_config.type,
                     position_dist(rng),
                     position_dist(rng),
                     width,
                     height,
                     {true, true, true, true},
                     {door_x_dist(rng), door_x_dist(rng), door_x_dist(rng), door_x_dist(rng)},
                     {door_y_dist(rng), door_y_dist(rng), door_y_dist(rng), door_y_dist(rng)},
                     room_config.attributes});
        }
    }

    return rooms;
}

void run_optimization(const std::vector<RoomConfig> &config) {}

TEST_CASE("generate_random_map()")
{
    SUBCASE("Quick tests")
    {
        const auto config = read_config_from_file("config.yml");
        const auto color_map = config_to_color_map(config);
        for (int i = 0; i < 10; i++)
        {
            const auto rooms = generate_random_rooms(config);
            const Map map(map_size, rooms);
            const auto bmp = map.to_bitmap(color_map);
            bmp.save_image(std::to_string(i) + ".bmp");
        }
    }
}
}