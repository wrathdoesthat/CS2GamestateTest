#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "json.hpp"

#include "sol.hpp"

#include "Config.hh"
#include "ConfigInteraction.hh"
#include "GamestateThread.hh"
#include "LogWatchThread.hh"

BOOL WINAPI console_handle_routine(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        spdlog::get("main")->critical("Ctrl-C aborting");
        std::exit(0);
    }
    return FALSE;
}

int main() {
    SetConsoleCtrlHandler(console_handle_routine, TRUE);

    auto logger = spdlog::stdout_color_mt("main");
    logger->set_level(spdlog::level::info); // spdlog::level::debug for everything

    std::ifstream config_file("./config.json");
    nlohmann::json parsed_file = nlohmann::json::parse(config_file);
    config_file.close();

    if (!parsed_file.contains("steam64_id")) {
        logger->info("config.json is missing steam64_id trying to read it from the registry");

        DWORD steam3_id;
        DWORD dwsize = sizeof(DWORD);
        LONG success = RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", "ActiveUser",
            RRF_RT_DWORD, 0, &steam3_id, &dwsize);

        if (success != ERROR_SUCCESS) {
            logger->critical("Couldnt get steamid from registry success={}", success);
            return 1;
        }

        uint64_t steam64_id = 76561197960265728 + steam3_id;
        parsed_file["steam64_id"] = steam64_id;

        logger->info("Steam id successfully read from registry steam64_id={}", std::to_string(steam64_id));
    }

    if (parsed_file.is_discarded()) {
        logger->critical("config.json failed to load");
        return 1;
    }

    if (!parsed_file.contains("port")) {
        logger->critical("config.json is missing the required port field (3009 was used for testing)");
        return 1;
    }

    if (!parsed_file.contains("logfile_path")) {
        logger->critical("config.json is missing the required logfile_path field");
        return 1;
    }

    if (parsed_file["logfile_path"] == "") {
        logger->critical("config.json logfile_path is empty field");
        return 1;
    }

    if (!std::filesystem::exists(parsed_file["logfile_path"])) {
        logger->critical("config.json logfile_path is empty field");
        return 1;
    }

    if (!parsed_file.contains("activation_char")) {
        logger->critical("config.json is missing the required activation_char field");
        return 1;
    }

    std::filesystem::path log_path{parsed_file["logfile_path"].get<std::string>() + "/console.log"};
    if (!std::filesystem::exists(log_path)) {
        logger->critical("Cant find your log at {}", log_path.string());
        logger->critical("Make sure you run your game with the -condebug launch option!", log_path.string());
        return 1;
    }

    // bind "\" "exec cs2ool"
    logger->warn("Make sure you have added bind \"{}\" \"exec cs2ool\" to your autoexec!",
        parsed_file["activation_char"].get<std::string>());

    nlohmann::json& config = get_config();
    config.update(parsed_file);

    // we copy the config in since we never allow it to change at runtime anyways
    std::jthread log_watch_thread(log_watch_thread_fn, config);
    std::jthread gamestate_thread(gamestate_thread_fn, config);

    gamestate_thread.join();
    log_watch_thread.join();
    return 0;
}