#pragma once

#include <deque>
#include <fcntl.h>
#include <format>
#include <fstream>
#include <io.h>
#include <string>
#include <thread>

#include <chrono>
using namespace std::chrono;
using namespace std::chrono_literals;

#include "ConfigInteraction.hh"
#include "json.hpp"
#include "spdlog/details/os.h"
#include "spdlog/spdlog.h"

#include "TlHelp32.h"
#include "Windows.h"

namespace LineEndings {
unsigned short LF = 10;
unsigned short CR = 13;
}; // namespace LineEndings

std::streampos last_read_size{};
void log_watch_thread_fn(nlohmann::json config) {
    auto logger = spdlog::get("main");

    std::wifstream log_file;
    log_file.open(config["logfile_path"].get<std::string>() + "/console.log", std::ios::binary);

    while (true) {
        // This whole method is bad and naive the game can write more than 1 line at a time to the file
        // if i ever want to handle stuff like getting information from "status" i need to seek the whole change
        // and read the lines accordingly across the whole change not just the final line

        // Wait for file to change size
        log_file.seekg(-1, std::ios_base::end);
        last_read_size = log_file.tellg();
        while (last_read_size == log_file.tellg()) {
            std::this_thread::sleep_for(1ms);

            // Seek back to potentially new end
            log_file.seekg(-1, std::ios_base::end);
        }

        if (log_file.peek() != LineEndings::LF) { // LF
            logger->warn("Log update missing LF at seekg-1 (game just open or close?)");
            log_file.seekg(0, std::ios_base::end); // Hopefully fixes seek-then-die thing
            continue;
        }
        log_file.seekg(-2, std::ios_base::end);

        if (log_file.peek() != LineEndings::CR) { // CR
            logger->warn("Log update missing CR at seekg -2 (game just open or close?)");
            log_file.seekg(0, std::ios_base::end); // Hopefully fixes seek-then-die thing
            continue;
        }

        bool string_found{false};
        std::vector<char> bytes{};

        // start seekg at first actual string byte
        for (int x{3}; x <= 203; x++) {
            log_file.seekg(-x, std::ios_base::end);
            unsigned short character = log_file.peek();

            if (character == LineEndings::LF) {
                log_file.seekg(x + 1);
                string_found = true;
                break;
            }

            bytes.push_back(static_cast<char>(character));
        }

        if (!string_found) {
            continue;
        }

        std::reverse(bytes.begin(), bytes.end());

        int wchars_num = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), NULL, 0);
        std::wstring wide_str;
        wide_str.resize(wchars_num);
        MultiByteToWideChar(CP_UTF8, 0, bytes.data(), static_cast<int>(bytes.size()), wide_str.data(), wchars_num);

        // Chat messages always have this "fake space" character next to the name for some reason
        std::size_t hidden_gem = wide_str.find(wchar_t(8206));
        if (hidden_gem == std::wstring::npos) { // It was not a message send to log handler instead
            continue;
        }

        // end of team thing
        std::size_t name_start = wide_str.find_first_of(L"]") + 2;
        std::wstring name(wide_str.begin() + name_start, wide_str.begin() + hidden_gem);
        std::size_t colon_pos = wide_str.find(L":", hidden_gem);

        bool was_team_chat{false};
        std::size_t team_start = wide_str.find_first_of(L"[");
        wchar_t team_char = wide_str.at(team_start + 1);
        if ((team_char == wchar_t(67)) || (team_char == wchar_t(84))) {
            was_team_chat = true;
        }

        std::wstring location{};
        if (was_team_chat) {
            /* 05/27 08:47:53  [T] testing‎﹫Silo: This isnt an ascii @ its some insane @﹫
            05/27 08:48:10  [ALL] testing‎: y */

            location = std::wstring(wide_str.begin() + hidden_gem + 2, wide_str.begin() + colon_pos);
        }

        std::wstring message_text(wide_str.begin() + (colon_pos + 2), wide_str.end());
    }
}