#include "threads.hpp"

#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>

#include "../../wrappers/number_value.hpp"
#include "../../wrappers/string_value.hpp"
#include "../../wrappers/service.hpp"
#include "../../helpers/console.hpp"
#include "../globals/globals.hpp"
#include "../../helpers/utils.hpp"

std::vector<int> players_val{};
int player_count = 0;
int mostfreq = 0;

std::mutex player_list_mutex;
std::vector<Player> player_list;

std::atomic<bool> loop_control(true);
std::atomic<bool> loop_running(true);
std::atomic<bool> is_restarting(false);
std::atomic<bool> has_started(false);
std::chrono::time_point<std::chrono::steady_clock> last_iteration_time;

std::thread list_thread;

std::vector<std::string> bone_names = {
     "Head", "LeftHand", "RightHand", "LeftLowerArm", "RightLowerArm",
     "LeftUpperArm", "RightUpperArm", "LeftFoot", "LeftLowerLeg",
     "UpperTorso", "LeftUpperLeg", "RightFoot", "RightLowerLeg",
     "LowerTorso", "RightUpperLeg"
};

int find_most_frequent(const std::vector<int>& nums) {
    if (nums.empty()) return -1;

    std::unordered_map<int, int> countMap;
    int maxCount = 0, mostFrequent = nums[0];

    for (int num : nums) {
        if (++countMap[num] > maxCount) {
            maxCount = countMap[num];
            mostFrequent = num;
        }
    }

    return mostFrequent;
}

template <typename T>
void sync_lists(std::vector<T>& namesList1, std::vector<T>& namesList2) {
    namesList1.erase(
        std::remove_if(
            namesList1.begin(),
            namesList1.end(),
            [&namesList2](const T& item) {
                return std::find(namesList2.begin(), namesList2.end(), item) == namesList2.end() || item.self == 0 || item.health == 0 || (memory::is_valid(item.self) && !is_valid_instance(item));
            }
        ),
        namesList1.end()
    );

    for (auto it = namesList2.begin(); it != namesList2.end(); ++it) {
        auto pos = std::distance(namesList2.begin(), it);
        if (pos >= namesList1.size() || *it != namesList1[pos])
            namesList1.insert(namesList1.begin() + pos, *it);
    }
}

void update_thread() {
    Service players = globals::game.players;

    while (true) {
        if (players.read_players() != mostfreq) {
            Sleep(125);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(player_list_mutex);
            std::vector<Player> current_players;

            if (globals::game_id != 3233893879 && globals::game_id != 292439477) {
                for (Player& entity : players.children<Player>()) {
                    Player plr;
                    plr.self = entity.self;
                    plr.static_instance = static_cast<Instance>(entity);

                    current_players.push_back(plr);

                    Sleep(1);
                }

                sync_lists(player_list, current_players);

                for (Player& entity : player_list) {
                    ModelInstance new_character = entity.character();

                    if (!is_valid_instance(entity.static_character, new_character))
                        entity.static_character = new_character;

                    Humanoid new_humanoid = entity.static_character.find_first_child<Humanoid>("Humanoid");

                    if (!is_valid_instance(entity.static_humanoid, new_humanoid))
                        entity.static_humanoid = new_humanoid;

                    Part new_root_part = entity.static_character.find_first_child<Part>("HumanoidRootPart");

                    if (!is_valid_instance(entity.static_root_part, new_root_part))
                        entity.static_root_part = new_root_part;

                    Part new_head = entity.static_character.find_first_child<Part>("Head");

                    if (!is_valid_instance(entity.static_head, new_head))
                        entity.static_head = new_head;

                    if (globals::game_id == 2788229376 || globals::game_id == 16033173781) {
                        Instance body_effects = entity.static_character.find_first_child("BodyEffects");

                        if (body_effects.self) {
                            if (globals::armor_bar)
                                entity.armor_path = body_effects.find_first_child<IntValue>("Armor");

                            if (globals::ko_check)
                                entity.ko_path = body_effects.find_first_child<BoolValue>("K.O");
                        }

                        if (globals::crew_check) {
                            StringValue found_crew = entity.static_instance.find_first_child("DataFolder").find_first_child("Information").find_first_child<StringValue>("Crew");

                            if (found_crew.self)
                                entity.crew = found_crew.value();
                        }
                    }

                    if (globals::skeleton && entity.static_character.self) {
                        for (Part& part : entity.static_character.children<Part>()) {
                            std::string part_name = part.name();

                            const auto it = std::find(bone_names.begin(), bone_names.end(), part_name);

                            if (it != bone_names.end())
                                entity.bones[part_name] = part;
                        }
                    }

                    entity.health = 100;
                    Sleep(1);
                }
            }
            else if (globals::game_id == 3233893879) {
                for (ModelInstance& character : globals::workspace.find_first_child("Characters").children<ModelInstance>()) {
                    Player plr;
                    plr.self = character.self;
                    plr.static_character = character;

                    current_players.push_back(plr);
                }

                sync_lists(player_list, current_players);

                for (Player& entity : player_list) {
                    entity.static_root_part = entity.static_character.find_first_child<Part>("Root");
                    entity.static_head = entity.static_character.find_first_child("Body").find_first_child<Part>("Head");

                    if (entity.static_root_part.self) {
                        NumberValue health = entity.static_character.find_first_child<NumberValue>("Health");

                        if (health.self)
                            entity.health = health.value();
                    }
                    else
                        entity.health = 150;
                }
            }
            else if (globals::game_id == 292439477) {
                for (Instance& team : globals::workspace.find_first_child("Players").children()) {
                    for (ModelInstance& character : team.children<ModelInstance>()) {
                        Player plr;

                        plr.self = character.self;
                        plr.static_character = character;
                        plr.health = 100;

                        current_players.push_back(plr);
                    }
                }

                player_list = current_players;

                for (Player& entity : player_list) {
                    for (Part& part : entity.children<Part>()) {
                        std::string part_class = part.class_name();

                        if (part_class == "Part" || part_class == "MeshPart") {
                            Vector3 part_size = part.size();

                            if (part_size == 1)
                                entity.static_head = part;
                            else if (part_size == Vector3{ 2, 2, 1 })
                                entity.static_root_part = part;
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
}