#include <vector>

#include <doctest/doctest.h>
#include <yaml-cpp/yaml.h>

#include "config.hpp"
#include "utils.hpp"

namespace rlo
{
std::unordered_map<unsigned char, rgb_t> config_to_color_map(const std::vector<RoomConfig> &config)
{
    std::unordered_map<unsigned char, rgb_t> color_map;
    color_map[floor] = {255, 255, 255};
    color_map[door] = {127, 127, 127};
    color_map[wall] = {0, 0, 0};

    for (const auto &room : config)
    {
        color_map[room.type] = room.color;
    }

    return color_map;
}

std::vector<RoomConfig> read_config_from_yaml(const YAML::Node &yaml)
{
    std::vector<RoomConfig> config;

    std::unordered_map<std::string, unsigned int> name_to_index;
    const auto size = yaml.size();
    for (unsigned int i = 0; i < size; i++)
    {
        name_to_index[yaml[i]["name"].as<std::string>()] = i;
    }

    for (const auto &room_yaml : yaml)
    {
        RoomConfig room{
            room_yaml["name"].as<std::string>(),
            static_cast<unsigned char>(name_to_index[room_yaml["name"].as<std::string>()]),
            room_yaml["count"].as<unsigned int>(),
            room_yaml["minimum_size"].as<unsigned int>(),
            room_yaml["size_scaling"].as<float>(),
            room_yaml["movement_cost"].as<float>(),
            {static_cast<unsigned char>(room_yaml["color"][0].as<unsigned int>()),
             static_cast<unsigned char>(room_yaml["color"][1].as<unsigned int>()),
             static_cast<unsigned char>(room_yaml["color"][2].as<unsigned int>())},
            {},
            {}};

        for (const auto &attribute : room_yaml["attributes"])
        {
            room.attributes.push_back(attribute.as<std::string>());
        }

        for (const auto &weight : room_yaml["weights"])
        {
            room.weights[name_to_index[weight.first.as<std::string>()]] =
                weight.second.as<float>();
        }

        config.push_back(room);
    }

    return config;
}

std::vector<RoomConfig> read_config_from_file(const std::string &file)
{
    const auto yaml = YAML::LoadFile(file);
    return read_config_from_yaml(yaml);
}

TEST_CASE("read_config_from_yaml()")
{
    const std::string yaml = R"(- name: bedroom
  count: 10
  minimum_size: 20
  size_scaling: 0.5
  movement_cost: 10
  color: [255, 255, 25]
  attributes:
    - grouped
  weights:
    stockpile: 0.02
- name: stockpile
  count: 1
  minimum_size: 100
  size_scaling: 2
  movement_cost: 2
  attributes: []
  color: [230, 25, 75]
  weights:
    bedroom: 0.5
    workshop: 5
- name: workshop
  count: 1
  minimum_size: 50
  size_scaling: 0.5
  movement_cost: 3
  attributes: []
  color: [60, 180, 75]
  weights:
    bedroom: 0.3
    stockpile: 5)";

    SUBCASE("Correctly loads config file")
    {
        auto config = read_config_from_yaml(YAML::Load(yaml));

        CHECK(config[0].name == "bedroom");
        CHECK(config[0].count == 10);
        CHECK(config[0].minimum_size == 20);
        CHECK(config[0].size_scaling == 0.5f);
        CHECK(config[0].movement_cost == 10.f);
        CHECK(config[0].color == rgb_t{255, 255, 25});
        CHECK(config[0].attributes == std::vector<std::string>{"grouped"});
        CHECK(config[0].weights[1] == 0.02f);
        CHECK(config[1].name == "stockpile");
        CHECK(config[1].count == 1);
        CHECK(config[1].minimum_size == 100);
        CHECK(config[1].size_scaling == 2.f);
        CHECK(config[1].movement_cost == 2.f);
        CHECK(config[1].color == rgb_t{230, 25, 75});
        CHECK(config[1].attributes == std::vector<std::string>{});
        CHECK(config[1].weights[0] == 0.5f);
        CHECK(config[1].weights[2] == 5.f);
        CHECK(config[2].name == "workshop");
        CHECK(config[2].count == 1);
        CHECK(config[2].minimum_size == 50);
        CHECK(config[2].size_scaling == 0.5f);
        CHECK(config[2].movement_cost == 3.f);
        CHECK(config[2].color == rgb_t{60, 180, 75});
        CHECK(config[2].attributes == std::vector<std::string>{});
        CHECK(config[2].weights[0] == 0.3f);
        CHECK(config[2].weights[1] == 5.f);
    }
}
}