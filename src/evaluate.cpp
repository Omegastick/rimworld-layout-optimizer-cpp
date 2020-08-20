#include <limits>
#include <tuple>
#include <vector>
#include <queue>

#include <doctest/doctest.h>

#include "bitmap.hpp"
#include "evaluate.hpp"
#include "config.hpp"
#include "map.hpp"
#include "utils.hpp"

namespace rlo
{
constexpr float door_cost = 1.f;
constexpr float door_move_cost = 25.f;
constexpr float wall_cost = 0.1f;

typedef std::vector<float> CostMap;

struct RoomInfo
{
    unsigned char type;
    long unsigned int size;
    unsigned int center_x;
    unsigned int center_y;
    unsigned int width;
    unsigned int height;
    std::vector<std::pair<unsigned int, unsigned int>> coordinates;
};

std::vector<std::pair<unsigned int, unsigned int>> flood_fill(std::vector<unsigned char> &map,
                                                              unsigned int map_size,
                                                              unsigned int start_x,
                                                              unsigned int start_y)
{
    std::vector<std::pair<unsigned int, unsigned int>> room_coordinates;
    const auto correct_type = map[start_y * map_size + start_x];

    std::queue<std::pair<unsigned int, unsigned int>> queue;
    queue.push(std::make_pair(start_x, start_y));

    while (!queue.empty())
    {
        const auto point = queue.front();
        queue.pop();

        if (point.first >= map_size || point.second >= map_size ||
            map[point.second * map_size + point.first] != correct_type)
        {
            continue;
        }

        room_coordinates.push_back(point);
        map[point.second * map_size + point.first] = floor;

        if (point.first > 0)
        {
            queue.push(std::make_pair(point.first - 1, point.second));
        }
        if (point.second > 0)
        {
            queue.push(std::make_pair(point.first, point.second - 1));
        }
        if (point.first < map_size)
        {
            queue.push(std::make_pair(point.first + 1, point.second));
        }
        if (point.second < map_size)
        {
            queue.push(std::make_pair(point.first, point.second + 1));
        }
    }

    return room_coordinates;
}

std::vector<RoomInfo> analyze_rooms(const Map &map)
{
    std::vector<RoomInfo> rooms;
    std::vector<unsigned char> temp_map(map.data());
    for (unsigned int x = 0; x < map.size(); x++)
    {
        for (unsigned int y = 0; y < map.size(); y++)
        {
            const auto tile = temp_map[y * map.size() + x];
            if (tile < floor)
            {
                const auto room_coordinates = flood_fill(temp_map, map.size(), x, y);
                unsigned int min_x = std::numeric_limits<unsigned int>::max();
                unsigned int min_y = std::numeric_limits<unsigned int>::max();
                unsigned int max_x = 0;
                unsigned int max_y = 0;
                for (const auto &coordinate : room_coordinates)
                {
                    min_x = std::min(coordinate.first, min_x);
                    min_y = std::min(coordinate.second, min_y);
                    max_x = std::max(coordinate.first, max_x);
                    max_y = std::max(coordinate.second, max_y);
                }
                rooms.emplace_back(RoomInfo{tile, room_coordinates.size(),
                                            room_coordinates[room_coordinates.size() / 2].first,
                                            room_coordinates[room_coordinates.size() / 2].second,
                                            max_x + 1 - min_x, max_y + 1 - min_y,
                                            room_coordinates});
            }
        }
    }
    return rooms;
}

CostMap create_costmap(const Map &map, const std::vector<RoomConfig> &config)
{
    CostMap cost_map(map.size() * map.size());

    for (unsigned int x = 0; x < map.size(); x++)
    {
        for (unsigned int y = 0; y < map.size(); y++)
        {
            unsigned char tile = map.get(x, y);

            if (tile == floor)
            {
                cost_map[y * map.size() + x] = 1.;
            }
            else if (tile == door)
            {
                cost_map[y * map.size() + x] = door_move_cost;
            }
            else if (tile == wall)
            {
                cost_map[y * map.size() + x] = std::numeric_limits<float>::infinity();
            }
            else
            {
                cost_map[y * map.size() + x] = config[tile].movement_cost;
            }
        }
    }

    return cost_map;
}

std::vector<float> distance_map(const CostMap &cost_map, unsigned int start_x,
                                unsigned int start_y, unsigned int map_size)
{
    std::vector<float> result(map_size * map_size, std::numeric_limits<float>::infinity());

    class Prioritize
    {
      public:
        bool operator()(std::pair<unsigned int, float> &p1, std::pair<unsigned int, float> &p2)
        {
            return p1.second > p2.second;
        }
    };

    std::priority_queue<std::pair<unsigned int, float>,
                        std::vector<std::pair<unsigned int, float>>, Prioritize>
        queue;
    std::vector<bool> visited(map_size * map_size, false);

    queue.push(std::make_pair(start_y * map_size + start_x, 0));

    while (!queue.empty())
    {
        const auto point = queue.top();
        queue.pop();

        if (visited[point.first])
        {
            continue;
        }
        visited[point.first] = true;

        const float cost = point.second + cost_map[point.first];
        result[point.first] = cost;

        if (point.first > map_size &&
            cost_map[point.first - map_size] != std::numeric_limits<float>::infinity())
        {
            queue.push(std::make_pair(point.first - map_size, cost));
        }
        if (point.first < map_size * map_size - map_size &&
            cost_map[point.first + map_size] != std::numeric_limits<float>::infinity())
        {
            queue.push(std::make_pair(point.first + map_size, cost));
        }
        if (point.first % map_size > 0 &&
            cost_map[point.first - 1] != std::numeric_limits<float>::infinity())
        {
            queue.push(std::make_pair(point.first - 1, cost));
        }
        if (point.first % map_size < map_size - 1 &&
            cost_map[point.first + 1] != std::numeric_limits<float>::infinity())
        {
            queue.push(std::make_pair(point.first + 1, cost));
        }
    }

    return result;
}

float evaluate(const Map &map, const std::vector<RoomConfig> &config)
{
    float score = 0.f;

    const auto cost_map = create_costmap(map, config);
    const auto room_infos = analyze_rooms(map);

    // Individual room operations
    for (const auto &room : room_infos)
    {
        if (room.size < 9)
        {
            score -= 100.f;
            continue;
        }
        const auto &room_config = config[room.type];

        // Size
        const auto max_room_size = room_config.minimum_size * 4;
        if (room.size < room_config.minimum_size)
        {
            score -= 1000.f;
        }
        else if (room.size < max_room_size)
        {
            score += static_cast<float>(room.size - room_config.minimum_size) *
                     room_config.size_scaling;
        }

        // Aspect ratio
        score -= static_cast<float>(
                     std::abs(static_cast<short>(room.width) - static_cast<short>(room.height))) *
                 10.f;
        if (room.width < 3 || room.height < 3)
        {
            score -= 100.f;
        }

        // Room shape
        // We calculate the expected area if the room was a rectangle, and any difference between
        // that and the actual area is considered bad.
        score -= static_cast<float>(room.width * room.height - static_cast<int>(room.size));

        // Distance to other rooms
        std::vector<float> temp_cost_map(cost_map);
        for (const auto &coordinate : room.coordinates)
        {
            temp_cost_map[coordinate.second * map.size() + coordinate.first] = 0.f;
        }
        const auto distances =
            distance_map(temp_cost_map, room.center_x, room.center_y, map.size());
        for (const auto &target_room : room_infos)
        {
            const auto weight = config[room.type].weights.find(target_room.type);
            if (weight != config[room.type].weights.end())
            {
                const auto cost =
                    distances[target_room.center_y * map.size() + target_room.center_x];
                if (cost == std::numeric_limits<float>::infinity())
                {
                    score -= 500;
                }
                else
                {
                    score -= cost * weight->second;
                }
            }
        }
    }

    // Global operations
    // Room count
    for (int i = 0; i < config.size(); i++)
    {
        unsigned int room_count = 0;
        for (const auto &room : room_infos)
        {
            if (room.size >= 9 && room.type == i)
            {
                room_count++;
            }
        }
        if (room_count != config[i].count)
        {
            score -= 15000.f * std::abs(static_cast<short>(room_count) -
                                        static_cast<short>(config[i].count));
        }
    }

    // Individual tiles
    for (const auto &tile : map.data())
    {
        if (tile == wall)
        {
            score -= wall_cost;
        }
        else if (tile == door)
        {
            score -= door_cost;
        }
    }

    return score;
}

TEST_CASE("analyze_rooms()")
{
    SUBCASE("Returns an empty vector if there are no rooms")
    {
        CHECK(analyze_rooms(Map(5, {})).size() == 0);
    }

    SUBCASE("Returns correct info for one room")
    {
        const auto map = Map(
            10,
            {Room{25, 1, 2, 3, 4, {true, false, true, false}, {0, 0, 2, 0}, {0, 0, 1, 0}, {}}});
        const auto room_info = analyze_rooms(map);

        CHECK(room_info[0].size == 2);
        CHECK(room_info[0].type == 25);
        CHECK(room_info[0].center_x == 2);
        CHECK(room_info[0].center_y == 4);
    }
}

TEST_CASE("create_costmap()")
{
    SUBCASE("A map with all floors should have cost 1 everywhere")
    {
        Map map(5, {});
        const auto cost_map = create_costmap(map, {});

        for (const auto &tile : cost_map)
        {
            CHECK(tile == 1.f);
        }
    }

    SUBCASE("Walls should have infinite cost")
    {
        const auto map = Map(
            10,
            {Room{25, 1, 2, 3, 4, {true, false, true, false}, {0, 0, 2, 0}, {0, 0, 1, 0}, {}}});
        std::vector<RoomConfig> config(26);
        const auto cost_map = create_costmap(map, config);

        CHECK(cost_map[3 * 10 + 1] == std::numeric_limits<float>::infinity());
    }

    SUBCASE("Doors have an appropriate cost")
    {
        const auto map = Map(
            10,
            {Room{25, 1, 2, 3, 4, {true, false, true, false}, {0, 0, 2, 0}, {0, 0, 1, 0}, {}}});
        std::vector<RoomConfig> config(26);
        const auto cost_map = create_costmap(map, config);

        CHECK(cost_map[2 * 10 + 1] == door_move_cost);
    }

    SUBCASE("Rooms have an appropriate cost")
    {
        const auto map = Map(
            10,
            {Room{25, 1, 2, 3, 4, {true, false, true, false}, {0, 0, 2, 0}, {0, 0, 1, 0}, {}}});
        std::vector<RoomConfig> config(26);
        config[25].movement_cost = 7.f;
        const auto cost_map = create_costmap(map, config);

        CHECK(cost_map[3 * 10 + 2] == 7.f);
    }
}
}