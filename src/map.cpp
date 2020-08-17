#include <vector>

#include <doctest/doctest.h>

#include "bitmap.hpp"
#include "map.hpp"

namespace rlo
{
Map::Map(unsigned int size)
{
    m_size = size;
    m_data = std::vector<unsigned char>(m_size * m_size, 0);
}

bitmap_image Map::to_bitmap(const std::unordered_map<unsigned char, rgb_t> &color_map) const
{
    bitmap_image image(m_size, m_size);

    for (unsigned int x = 0; x < m_size; x++)
    {
        for (unsigned int y = 0; y < m_size; y++)
        {
            image.set_pixel(x, y, color_map.at(m_data[y * x + y]));
        }
    }

    return image;
}

TEST_CASE("Map")
{
    SUBCASE("to_bitmap()")
    {
        SUBCASE("When called on a blank map, should produce a white image")
        {
            const auto map = Map(5);
            const auto bmp = map.to_bitmap({{0, rgb_t{255, 255, 255}}});

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