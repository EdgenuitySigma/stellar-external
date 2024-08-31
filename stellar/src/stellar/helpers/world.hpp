#pragma once

#ifndef WORLD_HPP
#define WORLD_HPP

#include <Windows.h>

#include "math.hpp"

#include "../cheat/globals/globals.hpp"
#include "../wrappers/player.hpp"
#include "../wrappers/part.hpp"
#include "../wrappers/bool_value.hpp"
#include "../cheat/threads/threads.hpp"

static Vector3 get_local_position() {
	if (!player_list.empty())
		return player_list.front().static_root_part.position();

	return { 0.f, 0.f, 0.f };
}

static struct wts_ret {
	Vector2 position;
	bool on_screen;
};

static wts_ret world_to_screen(Vector3 world) {
	Vector2 dimensions = globals::visual_engine.dimensions();
	Matrix4 view_matrix = globals::visual_engine.view_matrix();
	
	Vector4 quaternion;

	quaternion.x = (world.x * view_matrix.data[0]) + (world.y * view_matrix.data[1]) + (world.z * view_matrix.data[2]) + view_matrix.data[3];
	quaternion.y = (world.x * view_matrix.data[4]) + (world.y * view_matrix.data[5]) + (world.z * view_matrix.data[6]) + view_matrix.data[7];
	quaternion.z = (world.x * view_matrix.data[8]) + (world.y * view_matrix.data[9]) + (world.z * view_matrix.data[10]) + view_matrix.data[11];
	quaternion.w = (world.x * view_matrix.data[12]) + (world.y * view_matrix.data[13]) + (world.z * view_matrix.data[14]) + view_matrix.data[15];

	if (quaternion.w < 0.1f)
		return {
			{ -1, -1 },
			false
		};

	float inv_w = 1.0f / quaternion.w;

	Vector3 ndc;
	ndc.x = quaternion.x * inv_w;
	ndc.y = quaternion.y * inv_w;
	ndc.z = quaternion.z * inv_w;

	return {
		{(dimensions.x / 2 * ndc.x) + (ndc.x + dimensions.x / 2),-(dimensions.y / 2 * ndc.y) + (ndc.y + dimensions.y / 2)},
		true
	};
}

static Player get_closest_player() {
	POINT cursor_point;
	GetCursorPos(&cursor_point);
	ScreenToClient(FindWindowA(0, "Roblox"), &cursor_point);

	Vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };

	Player closest_player{};
	float closest_distance = 9e9;

	if (globals::fov_enabled)
		closest_distance = globals::fov;

	for (Player& entity : player_list) {
		if (!entity.self && entity.self != globals::game.players.local_player().self)
			continue;

		Vector3 part_pos_3d = entity.static_root_part.position();

		if (get_local_position() == part_pos_3d || part_pos_3d == 0)
			continue;

		if (entity.health < 1 || (globals::crew_check && player_list.front().crew && entity.crew == player_list.front().crew))
			continue;

		if (globals::ko_check && entity.ko_path.value())
			continue;

		auto [part_pos, on_screen] = world_to_screen(part_pos_3d);

		if (!on_screen)
			continue;

		float dist = calculate_distance(part_pos, cursor);

		if (closest_distance > dist && entity.self && entity.static_character.self) {
			closest_player = entity;
			closest_distance = dist;
		}
	}

	cursor = {};
	cursor_point = {};
	closest_distance = 0;

	return closest_player;
}

#endif