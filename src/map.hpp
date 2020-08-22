#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include <unordered_map>

#include "bitmap.hpp"

namespace rlo
{
struct Room
{
    unsigned char type;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    std::array<bool, 4> doors_active;
    std::array<unsigned int, 4> door_xs;
    std::array<unsigned int, 4> door_ys;
    std::vector<std::string> attributes;
};

struct Node
{
    unsigned int x;
    unsigned int y;
    unsigned char type;
    std::array<unsigned int, 4> door_positions;
};

class Map
{
  private:
    unsigned int m_size;
    std::vector<unsigned char> m_data;

    void make_tree(std::pair<unsigned int, unsigned int> boundary_tl,
                   std::pair<unsigned int, unsigned int> boundary_br, std::vector<Node> &nodes,
                   std::size_t begin, std::size_t end);

    inline void set(unsigned int x, unsigned int y, unsigned char value)
    {
        m_data.at(y * m_size + x) = value;
    }

  public:
    Map(unsigned int size, const std::vector<Room> &rooms);
    Map(const std::vector<unsigned char> &data);
    Map(unsigned int size, const std::vector<Node> &nodes);

    bitmap_image to_bitmap(const std::unordered_map<unsigned char, rgb_t> &color_map) const;

    inline const std::vector<unsigned char> &data() const { return m_data; }
    inline unsigned int size() const { return m_size; }
    inline unsigned char get(unsigned int x, unsigned int y) const
    {
        return m_data.at(y * m_size + x);
    }
};
}