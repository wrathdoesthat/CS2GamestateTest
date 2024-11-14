#include "ConfigInteraction.hh"

#include <filesystem>
#include <format>

#include "Windows.h"

#include <chrono>
#include <thread>
#include <queue>

using namespace std::chrono;
using namespace std::chrono_literals;

std::filesystem::path cached_path{""};
WORD cached_vkey{};

std::wofstream config_file{};

// Anything that must be written in chat 
system_clock::time_point last_say_time{};
std::queue<std::wstring> say_queue{};


void execute_command(std::wstring_view text) {
    if (cached_path == "") {
        cached_path = std::format("{}{}", get_config()["logfile_path"].get<std::string>(), "/cfg/cs2ool.cfg");
        cached_vkey = VkKeyScan(get_config()["activation_char"].get<std::string>().c_str()[0]); // lol
    }

    config_file.open(cached_path, std::ios::trunc);
    config_file << text;
    config_file.close();

    INPUT inputs[2] = {};
    std::memset(inputs, 0, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = cached_vkey;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = cached_vkey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

void write_all_chat(std::wstring_view text) {
    execute_command(std::format(L"say \"{}\"", text));
}

void write_team_chat(std::wstring_view text) {
    execute_command(std::format(L"say_team \"{}\"", text));
}

void write_console(std::wstring_view text) {
    execute_command(std::format(L"echo \"{}\"", text));
}