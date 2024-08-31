#pragma once

#ifndef STELLAR_HPP
#define STELLAR_HPP

#include <cstdint>

namespace stellar {
    bool init();
    void close();

    namespace offsets {
        constexpr std::uint32_t size = 0x8;
        constexpr std::uint32_t name = 0x48;

        constexpr std::uint32_t displayname = 0xD8;
        constexpr std::uint32_t children = 0x50;
        constexpr std::uint32_t parent = 0x60;
        constexpr std::uint32_t local_player = 0x100;
        constexpr std::uint32_t model_instance = 0x298;
        constexpr std::uint32_t primitive = 0x150;
        constexpr std::uint32_t part_size = 0x27C;
        constexpr std::uint32_t position = 0x140;
        constexpr std::uint32_t dimensions = 0x720;
        constexpr std::uint32_t move_direction = 0x370;
        constexpr std::uint32_t viewmatrix = 0x4B0;
        constexpr std::uint32_t classname = 0x18;
        constexpr std::uint32_t health = 0x174;
        constexpr std::uint32_t max_health = 0x194;
        constexpr std::uint32_t walkspeed = 0x340;
        constexpr std::uint32_t jumpspeed = 0x370;
        constexpr std::uint32_t team = 0x1D8;
        constexpr std::uint32_t gameid = 0x168;
        constexpr std::uint32_t velocity = 0x14C;
        constexpr std::uint32_t camera_pos = 0xF4;
        constexpr std::uint32_t camera = 0x418;
        constexpr std::uint32_t camera_rotation = 0xD0;
        constexpr std::uint32_t walkspeed_a = 0x1B0;
        constexpr std::uint32_t walkspeed_b = 0x388;
        constexpr std::uint32_t jumppower = 0x018C;
        constexpr std::uint32_t hipheight = 0x08C;
        constexpr std::uint32_t ping = 0xB8;

        constexpr std::uint32_t value = 0xC0;

        constexpr std::uint32_t datamodel1 = 0x118;
        constexpr std::uint32_t datamodel2 = 0x198;

        constexpr std::uint32_t rotation = 0x11C;
    }
}




#endif