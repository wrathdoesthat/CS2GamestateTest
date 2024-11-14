#pragma once

#include <variant>
#include <string>
#include <functional>
#include <map>

/* 
    Events we expect to handle
    Log updating
    Messages being sent
    Should set up a "command" handler
    Key bound commands?

    State updates

*/

enum class CallbackEvent {
    OnMessage, // Called for any message
    OnLog, // Called for any log
    OnGamestateUpdate, // Called on any update of the gamestate
    OnPlayerDeath, // Called when our player dies
};

struct UserText {
    uint64_t steam_id{};
    std::wstring text{};
};

struct Text {
    std::wstring text{};
};

void register_callback(CallbackEvent event, std::function<void(void*)> fn);