#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include "json.hpp"

// Try to have a good initial state and mostly use the "added" and "previously" to update
nlohmann::json initial_gamestate = {
{"map", {
    {"mode", ""},
    {"name", ""},
    {"num_matches_to_win_series", 0},
    {"phase", ""},
    {"round", 0},
    {"current_spectators", 0},
	{"souvenirs_total", 0},
    {"team_ct", {
        {"consecutive_round_losses", 0},
        {"matches_won_this_series", 0},
        {"score", 0},
        {"timeouts_remaining", 0}
    }},
    {"team_t", {
        {"consecutive_round_losses", 0},
        {"matches_won_this_series", 0},
        {"score", 0},
        {"timeouts_remaining", 0}
    }},
    {"round_wins", {}}
}},
{"player", {
    {"activity", ""},
    {"match_stats", {
        {"assists", 0},
        {"deaths", 0},
        {"kills", 0},
        {"mvps", 0},
        {"score", 0}
    }},
    {"name", ""},
    {"observer_slot", 0},
    {"state", {
        {"armor", 0},
        {"burning", 0},
        {"equip_value", 0},
        {"flashed", 0},
        {"health", 0},
        {"helmet", false},
        {"money", 0},
        {"round_killhs", 0},
        {"round_kills", 0},
        {"smoked", 0}
    }},
    {"steamid", ""},
    {"clan", ""},
    {"team", ""},
    {"spectarget", ""},
    {"position", "-"},
    {"forward", ""},
    {"weapons", {}}
}},
{"bomb", {
	{"state", ""},
	{"position", ""},
	{"player", 0},
    {"countdown", ""}
}},
{"round", {
	{"phase", ""},
	{"win_team", ""},
	{"bomb", ""}
}},
{"phase_countdowns", {
	{"phase", "0"},
	{"phase_ends_in", "0"}
}},
};

nlohmann::json gamestate = initial_gamestate;

void update_state(nlohmann::json& state, const nlohmann::json& new_state) {
    state.update(new_state);
};