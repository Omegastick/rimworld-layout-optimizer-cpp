#include <vector>

#include <doctest/doctest.h>

#include "bitmap.hpp"
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
                    y == room.y + room.height - 1)
                {
                    set(x, y, wall);
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
                set(room.x + room.door_xs[i], room.y + room.door_ys[i], door);
            }
        }
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
            const auto map = Map(5, {});

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
            const auto map = Map(
                10,
                {Room{
                    25, 1, 2, 3, 4, {false, false, false, false}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}}});

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
            const auto map = Map(
                10,
                {Room{
                    25, 1, 2, 3, 4, {false, false, false, false}, {0, 0, 0, 0}, {0, 0, 0, 0}, {}}});

            CHECK(map.get(2, 3) == 25);
            CHECK(map.get(2, 4) == 25);
        }

        SUBCASE("Places doors in correct locations")
        {
            const auto map = Map(
                10,
                {Room{25, 1, 2, 3, 4, {true, false, true, false}, {0, 0, 2, 0}, {0, 0, 1, 0}, {}}});

            CHECK(map.get(1, 2) == door);
            CHECK(map.get(3, 3) == door);

            // const auto bmp = map.to_bitmap({{floor, {255, 255, 255}},
            //                                 {wall, {0, 0, 0}},
            //                                 {door, {127, 127, 127}},
            //                                 {25, {100, 0, 100}}});
            // bmp.save_image("test.bmp");
        }
    }

    SUBCASE("to_bitmap()")
    {
        SUBCASE("When called on a blank map, should produce a white image")
        {
            const auto map = Map(5, {});
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