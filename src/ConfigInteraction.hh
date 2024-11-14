#pragma once

#include <fstream>
#include <string_view>

#include "Config.hh"

void execute_command(std::wstring_view text);
void write_all_chat(std::wstring_view text);
void write_team_chat(std::wstring_view text);
void write_console(std::wstring_view text);