#include <random>
#include <vector>

#include <doctest/doctest.h>

#include "bitmap.hpp"
#include "config.hpp"
#include "map.hpp"
#include "utils.hpp"

namespace rlo
{
Map::Map(unsigned int size, const std::vector<Room> &rooms)
{
    m_size = size;
    m_data = std::vector<unsigned char>(m_size * m_size, floor);

    for (const auto &room : rooms)
    {
        for (unsigned int x = room.x; x < room.x + room.width; x++)
        {
            for (unsigned int y = room.y; y < room.y + room.height; y++)
            {
                if (x == room.x || y == room.y || x == room.x + room.width - 1 ||
                    y == room.y + room.height - 1 || x >= m_size || y >= m_size)
                {
                    set(std::min(x, m_size - 1), std::min(y, m_size - 1), wall);
                }
                else
                {
                    set(x, y, room.type);
                }
            }
        }

        for (unsigned int i = 0; i < 4; i++)
        {
            if (room.doors_active[i])
            {
                set(std::min(room.x + room.door_xs[i], m_size - 1),
                    std::min(room.y + room.door_ys[i], m_size - 1), door);
            }
        }
    }
}

Map::Map(const std::vector<unsigned char> &data)
{
    m_size = static_cast<unsigned int>(std::sqrt(data.size()));
    m_data = data;
}

Map::Map(unsigned int size, const std::vector<Node> &nodes)
{
    // K-D tree map construction
    m_size = size;
    m_data = std::vector<unsigned char>(m_size * m_size, floor);

    auto nodes_copy = nodes;
    make_tree({0, 0}, {size - 1, size - 1}, nodes_copy, 0, nodes.size());
}

void Map::make_tree(std::pair<unsigned int, unsigned int> boundary_tl,
                    std::pair<unsigned int, unsigned int> boundary_br, std::vector<Node> &nodes,
                    std::size_t begin, std::size_t end)
{
    if (end - begin == 1 || (end - begin == 2 && nodes[begin].x == nodes[begin + 1].x &&
                             nodes[begin].y == nodes[begin + 1].y))
    {
        if (nodes[begin].type == floor)
        {
            return;
        }
        for (unsigned int x = boundary_tl.first + 1; x < boundary_br.first; x++)
        {
            for (unsigned int y = boundary_tl.second + 1; y < boundary_br.second; y++)
            {
                set(x, y, nodes[begin].type);
            }
        }
        set(boundary_br.first, boundary_br.second, wall);
        std::sort(nodes[begin].door_positions.begin(), nodes[begin].door_positions.end());
        int i = 0;
        int door_index = 0;
        for (unsigned int x = boundary_tl.first; x < boundary_br.first; x++)
        {
            if (door_index < 4 && i++ == nodes[begin].door_positions[door_index])
            {
                set(x, boundary_tl.second, door);
                door_index++;
            }
            else
            {
                set(x, boundary_tl.second, wall);
            }
        }
        for (unsigned int y = boundary_tl.second; y < boundary_br.second; y++)
        {
            if (door_index < 4 && i++ == nodes[begin].door_positions[door_index])
            {
                set(boundary_tl.first, y, door);
                door_index++;
            }
            else
            {
                set(boundary_tl.first, y, wall);
            }
        }
        for (unsigned int x = boundary_tl.first; x < boundary_br.first; x++)
        {
            if (door_index < 4 && i++ == nodes[begin].door_positions[door_index])
            {
                set(x, boundary_br.second, door);
                door_index++;
            }
            else
            {
                set(x, boundary_br.second, wall);
            }
        }
        for (unsigned int y = boundary_tl.second; y < boundary_br.second; y++)
        {
            if (door_index < 4 && i++ == nodes[begin].door_positions[door_index])
            {
                set(boundary_br.first, y, door);
                door_index++;
            }
            else
            {
                set(boundary_br.first, y, wall);
            }
        }
        return;
    }

    std::size_t size = end - begin;
    unsigned int x_min = std::numeric_limits<unsigned int>::max();
    unsigned int y_min = std::numeric_limits<unsigned int>::max();
    unsigned int x_max = 0;
    unsigned int y_max = 0;
    float x_mean = 0;
    float y_mean = 0;
    for (std::size_t i = begin; i < end; i++)
    {
        x_min = std::min(x_min, nodes[i].x);
        y_min = std::min(y_min, nodes[i].y);
        x_max = std::max(x_max, nodes[i].x);
        y_max = std::max(y_max, nodes[i].y);
        x_mean += static_cast<float>(nodes[i].x);
        y_mean += static_cast<float>(nodes[i].y);
    }
    x_mean /= static_cast<float>(size);
    y_mean /= static_cast<float>(size);
    if (x_max - x_min > y_max - y_min)
    {
        std::sort(nodes.begin() + begin, nodes.begin() + end,
                  [](const auto &lhs, const auto &rhs) { return lhs.x < rhs.x; });
        const auto midpoint = static_cast<unsigned int>(std::round(x_mean));
        unsigned int nodes_below = 0;
        for (std::size_t i = begin; i < end; i++)
        {
            if (static_cast<float>(nodes[i].x) > x_mean)
            {
                break;
            }
            nodes_below++;
        }
        make_tree(boundary_tl, {midpoint, boundary_br.second}, nodes, begin, begin + nodes_below);
        make_tree({midpoint, boundary_tl.second}, boundary_br, nodes, begin + nodes_below, end);
    }
    else
    {
        std::sort(nodes.begin() + begin, nodes.begin() + end,
                  [](const auto &lhs, const auto &rhs) { return lhs.y < rhs.y; });
        const auto midpoint = static_cast<unsigned int>(std::round(y_mean));
        unsigned int nodes_below = 0;
        for (std::size_t i = begin; i < end; i++)
        {
            if (static_cast<float>(nodes[i].y) > y_mean)
            {
                break;
            }
            nodes_below++;
        }
        make_tree(boundary_tl, {boundary_br.first, midpoint}, nodes, begin, begin + nodes_below);
        make_tree({boundary_tl.first, midpoint}, boundary_br, nodes, begin + nodes_below, end);
    }
}

bitmap_image Map::to_bitmap(const std::unordered_map<unsigned char, rgb_t> &color_map) const
{
    bitmap_image image(m_size, m_size);

    for (unsigned int x = 0; x < m_size; x++)
    {
        for (unsigned int y = 0; y < m_size; y++)
        {
            image.set_pixel(x, y, color_map.at(m_data[y * m_size + x]));
        }
    }

    return image;
}

TEST_CASE("Map")
{
    SUBCASE("Map()")
    {
        SUBCASE("Is blank without any rooms")
        {
            const auto map = Map(5, std::vector<Room>{});

            for (unsigned int x = 0; x < 5; x++)
            {
                for (unsigned int y = 0; y < 5; y++)
                {
                    CHECK(map.get(x, y) == floor);
                }
            }
        }

        SUBCASE("Places walls around a room")
        {
            const auto map = Map(10, {Room{25,
                                           1,
                                           2,
                                           3,
                                           4,
                                           {false, false, false, false},
                                           {0, 0, 0, 0},
                                           {0, 0, 0, 0},
                                           {}}});

            CHECK(map.get(1, 2) == wall);
            CHECK(map.get(1, 3) == wall);
            CHECK(map.get(1, 4) == wall);
            CHECK(map.get(1, 5) == wall);
            CHECK(map.get(2, 2) == wall);
            CHECK(map.get(3, 2) == wall);
            CHECK(map.get(2, 5) == wall);
            CHECK(map.get(3, 5) == wall);
            CHECK(map.get(3, 3) == wall);
            CHECK(map.get(3, 4) == wall);
        }

        SUBCASE("Fills in room with the relevant tiles")
        {
            const auto map = Map(10, {Room{25,
                                           1,
                                           2,
                                           3,
                                           4,
                                           {false, false, false, false},
                                           {0, 0, 0, 0},
                                           {0, 0, 0, 0},
                                           {}}});

            CHECK(map.get(2, 3) == 25);
            CHECK(map.get(2, 4) == 25);
        }

        SUBCASE("Places doors in correct locations")
        {
            const auto map = Map(
                10,
                {Room{
                    25, 1, 2, 3, 4, {true, false, true, false}, {0, 0, 2, 0}, {0, 0, 1, 0}, {}}});

            CHECK(map.get(1, 2) == door);
            CHECK(map.get(3, 3) == door);
        }
    }

    SUBCASE("K-D Tree")
    {
        const auto config = read_config_from_file("config.yml");
        const auto color_map = config_to_color_map(config);

        std::random_device dev;
        std::mt19937 rng(dev());
        std::vector<Node> nodes;
        for (int i = 0; i < 100; i++)
        {
            if (std::uniform_int_distribution<int>(0, 1)(rng))
            {
                nodes.push_back(
                    {std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                     std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                     std::uniform_int_distribution<unsigned char>(0, config.size() - 1)(rng),
                     {std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                      std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                      std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                      std::uniform_int_distribution<unsigned int>(0, 99)(rng)}});
            }
            else
            {
                nodes.push_back({std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                                 std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                                 floor,
                                 {std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                                  std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                                  std::uniform_int_distribution<unsigned int>(0, 99)(rng),
                                  std::uniform_int_distribution<unsigned int>(0, 99)(rng)}});
            }
        }

        // std::vector<Node> nodes{{25, 25, floor}, {25, 75, door}, {90, 50, 25}};
        Map map(100, nodes);
        const auto bmp = map.to_bitmap(color_map);
        bmp.save_image("test.bmp");
    }

    SUBCASE("to_bitmap()")
    {
        SUBCASE("When called on a blank map, should produce a white image")
        {
            const auto map = Map(5, std::vector<Room>{});
            const auto bmp = map.to_bitmap({{floor, rgb_t{255, 255, 255}}});

            for (unsigned int x = 0; x < 5; x++)
            {
                for (unsigned int y = 0; y < 5; y++)
                {
                    CHECK(bmp.get_pixel(x, y) == rgb_t({255, 255, 255}));
                }
            }
        }
    }
}
}