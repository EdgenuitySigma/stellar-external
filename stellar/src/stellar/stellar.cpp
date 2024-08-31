#include "stellar.hpp"

#include <ShlObj.h>
#include <mutex>

#include "wrappers/datamodel.hpp"
#include "wrappers/instance.hpp"
#include "wrappers/service.hpp"

#include "driver/driver.hpp"
#include "driver/kernel_ctx/kernel_ctx.h"
#include "driver/drv_image/drv_image.h"
#include "driver/raw_driver.hpp"
#include "driver/driver_data.hpp"

#include "helpers/console.hpp"
#include "wrappers/visual_engine.hpp"
#include "cheat/threads/threads.hpp"
#include "cheat/globals/globals.hpp"
#include "helpers/world.hpp"
#include "cheat/features/triggerbot/triggerbot.hpp"

#include "cheat/features/aimbot/aimbot.hpp"
#include "cheat/features/anti_lock/anti_lock.hpp"
#include "cheat/overlay/overlay.hpp"
#include "helpers/utils.hpp"
std::mutex update_mutex;

static std::wstring appdata_path() {
	wchar_t path[MAX_PATH];

	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path)))
		return std::wstring(path);

	return L"";
}

std::vector<std::filesystem::path> get_roblox_file_logs() {
	std::vector<std::filesystem::path> roblox_log;
	std::wstring app_data_path = appdata_path();
	std::wstring roblox_log_path = app_data_path + L"\\Roblox\\logs";

	for (const auto& entry : std::filesystem::directory_iterator(roblox_log_path))
		if (entry.is_regular_file() && entry.path().extension() == ".log" && entry.path().filename().string().find("Player") != std::string::npos)
			roblox_log.push_back(entry.path());

	return roblox_log;
}

std::filesystem::path get_latest_log() {
	auto logs = get_roblox_file_logs();

	std::sort(logs.begin(), logs.end(), [](const std::wstring& a, const std::wstring& b) {
		return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
	});

	return logs[0];
}


static std::uint64_t get_render_view() {
	auto latest_log = get_latest_log();

	std::ifstream rbx_log(latest_log);
	std::string rbx_log_line;

	while (true) {
		std::getline(rbx_log, rbx_log_line);
		if (rbx_log_line.contains("initialize view(")) {
			rbx_log_line = rbx_log_line.substr(rbx_log_line.find("initialize view(") + 21);
			rbx_log_line = rbx_log_line.substr(0, rbx_log_line.find(')'));

			std::uint64_t renderview = std::strtoull(rbx_log_line.c_str(), nullptr, 16);
			return renderview;
		}
	}
}

bool stellar::init() {
    bool driver_fs = false;

    if (!memory::find_driver()) {
        driver_fs = true;
        COUT("Initializing driver...");

        std::vector<std::uint8_t> drv_buffer(WindowsFixService11, WindowsFixService10 + sizeof(WindowsFixService10));
        if (!drv_buffer.size()) {
            COUT("Invalid drv_buffer size!");
            return false;
        }

        physmeme::drv_image image(drv_buffer);
        if (!physmeme::load_drv()) {
            COUT("Failed to load driver!");
            return false;
        }

        const auto drv_timestamp = util::get_file_header((void*)raw_driver)->TimeDateStamp;

        physmeme::kernel_ctx kernel_ctx;
        const auto _get_export_name = [&](const char* base, const char* name) {
            return reinterpret_cast<std::uintptr_t>(util::get_kernel_export(base, name));
            };

        image.fix_imports(_get_export_name);
        image.map();

        const auto pool_base = kernel_ctx.allocate_pool(image.size(), NonPagedPool);
        image.relocate(pool_base);
        kernel_ctx.write_kernel(pool_base, image.data(), image.size());
        auto entry_point = reinterpret_cast<std::uintptr_t>(pool_base) + image.entry_point();

        auto result = kernel_ctx.syscall<DRIVER_INITIALIZE>(
            reinterpret_cast<void*>(entry_point),
            reinterpret_cast<std::uintptr_t>(pool_base),
            image.size()
        );
    }

    if (!memory::find_driver()) {
        COUT("Driver failed to initialize. Please restart your PC");
        return false;
    }

    memory::process_id = memory::find_process("RobloxPlayerBeta.exe");

    if (!memory::process_id) {
        if (driver_fs)
            CNEWLINE;

        COUT("Waiting for Roblox...");

        while (!memory::process_id) {
            memory::process_id = memory::find_process("RobloxPlayerBeta.exe");
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    globals::game_handle = FindWindowA(0, "Roblox");

    if (!globals::game_handle) {
        COUT("Failed to get game handle!");
        return false;
    }

    COUT("Found Roblox! Waiting 3 seconds before initialization...");
    CNEWLINE;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    /////////////////////////

    std::uint64_t render_view = get_render_view();
    if (!render_view) {
        COUT("Failed to get Render View!");
        return false;
    }

    VisualEngine visual_engine = static_cast<VisualEngine>(read<std::uint64_t>(render_view + 0x10));

    if (!visual_engine.self) {
        COUT("Failed to get Visual Engine!");
        return false;
    }

    std::uint64_t game_ptr = read<std::uint64_t>(render_view + stellar::offsets::datamodel1);

    if (!game_ptr) {
        COUT("Failed to get DataModel [1]! Cheat might not be updated");
        return false;
    }

    DataModel game = read<DataModel>(game_ptr + stellar::offsets::datamodel2);

    if (!game.self) {
        COUT("Failed to get DataModel [2]! Cheat might not be updated");
        return false;
    }

    globals::game_id = game.game_id();

    if (globals::game_id != 18461536252) {
        Service players = game.find_first_child<Service>("Players");

        if (!players.self) {
            COUT("Failed to get Player service!");
            return false;
        }

        game.players = players;

        for (int i = 0; i < 30; i++)
            players_val.push_back(players.read_players());

        mostfreq = find_most_frequent(players_val);
    }

    globals::workspace = game.find_first_child<Workspace>("Workspace");
    globals::visual_engine = visual_engine;
    globals::game = game;

    std::thread(stellar::cheats::aimbot).detach();
    std::thread(stellar::cheats::triggerbot).detach();
    std::thread(stellar::cheats::anti_lock).detach();
    std::thread(stellar::overlay::render).detach();
    std::thread(update_thread).detach();

    COUT("Initialized!");

    while (true) {
        globals::foreground = GetForegroundWindow();
        globals::is_focused = globals::foreground == globals::game_handle;

        if (!memory::find_process("RobloxPlayerBeta.exe")) {
            COUT("Roblox Closed Closing Stellar");
            stellar::close();
            return false;
        }

        std::this_thread::sleep_for(std::chrono::seconds(6));
    }

    /////////////////////////

    return true;
}

void stellar::close() {
    exit(-1);
}