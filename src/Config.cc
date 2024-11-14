#include "Config.hh"

static nlohmann::json config {
    {"port", 3009},
    {"logfile_path", ""},
    {"activation_char", "\\"}
};

nlohmann::json& get_config() {
    return config;
}