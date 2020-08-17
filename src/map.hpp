#pragma once

#include <vector>
#include <unordered_map>

#include "bitmap.hpp"

namespace rlo
{
class Map
{
  private:
    unsigned int m_size;
    std::vector<unsigned char> m_data;

  public:
    Map(unsigned int size);

    bitmap_image to_bitmap(const std::unordered_map<unsigned char, rgb_t> &color_map) const;

    inline const std::vector<unsigned char> &data() const { return m_data; }
};
}