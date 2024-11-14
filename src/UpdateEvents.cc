#include "UpdateEvents.hh"
#include "sol.hpp"

struct EventCallbacks {
    std::vector<std::function<void(void*)>> native_callbacks{};
    std::vector<sol::function> lua_callbacks{};
};

std::unordered_map<CallbackEvent, EventCallbacks> registered_callbacks{};

void register_callback(CallbackEvent event, std::function<void(void*)> fn) {
    registered_callbacks[event].native_callbacks.push_back(fn);
};

void register_callback(CallbackEvent event, sol::function fn) {
    registered_callbacks[event].lua_callbacks.push_back(fn);
};

void fire_callbacks(CallbackEvent event) {
    const auto& callbacks = registered_callbacks[event];
    for (const auto& cb : callbacks.native_callbacks) {
        
    }
    for (const auto& cb : callbacks.lua_callbacks) {
    }
};