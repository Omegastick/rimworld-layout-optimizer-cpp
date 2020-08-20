#include <algorithm>
#include <future>
#include <iostream>
#include <random>

#include <doctest/doctest.h>

#include "optimize.hpp"
#include "config.hpp"
#include "evaluate.hpp"
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
        for (unsigned int i = 0; i < room_config.count; i++)
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

template <class Generator>
std::vector<Room> permute(const std::vector<Room> &input, Generator &rng)
{
    std::vector<Room> output(input);

    const auto choice = std::uniform_real_distribution<double>()(rng);
    // Apply a random adjustment to a single room
    if (choice > 0.05)
    {
        const unsigned int room_index = std::uniform_int_distribution<unsigned int>(
            0, static_cast<unsigned int>(output.size()) - 1)(rng);
        auto &room = output[room_index];

        const auto move_type = std::uniform_int_distribution<unsigned int>(0, 5)(rng);
        int move_amount;
        unsigned int door_choice;
        switch (move_type)
        {
        case 0: // X
            move_amount =
                static_cast<int>(std::round(std::normal_distribution<double>(0, 3)(rng)));
            room.x = std::clamp(static_cast<int>(room.x) + move_amount, 0,
                                static_cast<int>(map_size) - 1);
            break;
        case 1: // Y
            move_amount =
                static_cast<int>(std::round(std::normal_distribution<double>(0, 3)(rng)));
            room.y = std::clamp(static_cast<int>(room.y) + move_amount, 0,
                                static_cast<int>(map_size) - 1);
            break;
        case 2: // Width
            move_amount =
                static_cast<int>(std::round(std::normal_distribution<double>(0, 3)(rng)));
            for (auto &door : room.door_xs)
            {
                if (door == room.width)
                {
                    door = std::clamp(static_cast<int>(door) + move_amount - 1, 4, 14);
                }
            }
            room.width = std::clamp(static_cast<int>(room.width) + move_amount, 4, 15);

            break;
        case 3: // Height
            move_amount =
                static_cast<int>(std::round(std::normal_distribution<double>(0, 3)(rng)));
            for (auto &door : room.door_ys)
            {
                if (door == room.height)
                {
                    door = std::clamp(static_cast<int>(room.height) + move_amount - 1, 4, 14);
                }
            }
            room.height = std::clamp(static_cast<int>(room.height) + move_amount, 4, 15);
            break;
        case 4: // Number of doors
            door_choice = std::uniform_int_distribution<unsigned int>(0, 3)(rng);
            room.doors_active[door_choice] = !room.doors_active[door_choice];
            break;
        case 5: // Door position
            door_choice = std::uniform_int_distribution<unsigned int>(0, 3)(rng);
            move_amount =
                static_cast<int>(std::round(std::normal_distribution<double>(0, 3)(rng)));
            const int horizontal = std::uniform_int_distribution<int>(0, 1)(rng);
            if (horizontal)
            {
                room.door_xs[door_choice] =
                    std::clamp(static_cast<int>(room.door_xs[door_choice]) + move_amount, 0,
                               static_cast<int>(room.width));
            }
            else
            {
                room.door_ys[door_choice] =
                    std::clamp(static_cast<int>(room.door_ys[door_choice]) + move_amount, 0,
                               static_cast<int>(room.height));
            }
            break;
        }
    }
    // Swap two rooms
    else
    {
        unsigned int choice_a = std::uniform_int_distribution<unsigned int>(
            0, static_cast<unsigned int>(output.size()) - 1)(rng);
        unsigned int choice_b = std::uniform_int_distribution<unsigned int>(
            0, static_cast<unsigned int>(output.size()) - 1)(rng);
        if (choice_a == choice_b)
        {
            if (choice_b > 0)
            {
                choice_b--;
            }
            else
            {
                choice_b++;
            }
        }
        const auto temp = output[choice_a].type;
        output[choice_a].type = output[choice_b].type;
        output[choice_b].type = temp;
    }

    return output;
}

void run_optimization(const std::vector<RoomConfig> &config)
{
    std::random_device device;

    const auto color_map = config_to_color_map(config);

    auto rooms = generate_random_rooms(config);
    float score = evaluate(Map(map_size, rooms), config);

    float threshold = 10000.f;

    for (int i = 0; i < 500; i++)
    {
        std::cout << std::to_string(static_cast<float>(i) / 500.f * 100.f) << "%\n";
        std::cout << "Threshold: " << std::to_string(threshold) << "\n";
        std::cout << "Score: " << std::to_string(score) << "\n";
        std::cout << "---\n";
        const auto bmp = Map(map_size, rooms).to_bitmap(color_map);
        bmp.save_image("output/" + std::to_string(i) + ".bmp");

        std::vector<std::future<std::pair<std::vector<Room>, float>>> futures;
        for (int thread = 0; thread < 16; thread++)
        {
            futures.emplace_back(std::async(
                [](std::vector<Room> starting_rooms, float starting_score,
                   std::vector<RoomConfig> config, float threshold, int seed) {
                    std::mt19937 rng(seed);

                    float score = starting_score;
                    auto rooms = starting_rooms;

                    for (int j = 0; j < 1000; j++)
                    {
                        const int number_of_permutations =
                            std::uniform_int_distribution<int>(1, 3)(rng);
                        auto new_rooms = rooms;
                        for (int i = 0; i < number_of_permutations; i++)
                        {
                            new_rooms = permute(rooms, rng);
                        }
                        float new_score = evaluate(Map(map_size, new_rooms), config);
                        if (score - new_score < threshold)
                        {
                            score = new_score;
                            rooms = new_rooms;
                        }
                    }

                    return std::make_pair(rooms, score);
                },
                rooms, score, config, threshold, device() + i));
        }

        score = -std::numeric_limits<float>::infinity();

        for (auto &future : futures)
        {
            const auto result = future.get();
            if (result.second > score)
            {
                rooms = result.first;
                score = result.second;
            }
        }

        threshold *= 0.9f;
    }

    std::cout << "100%\n";
    std::cout << "Threshold: " << std::to_string(threshold) << "\n";
    std::cout << "Score: " << std::to_string(score) << "\n";
    std::cout << "---\n";
    const auto bmp = Map(map_size, rooms).to_bitmap(color_map);
    bmp.save_image("output/final.bmp");
}
}