#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

namespace model_chat {

struct Config {
    std::string server_url;
    std::string model;
    int context_size = 1024;
    int max_tokens = 64;
    double temperature = 0.2;
    int connect_timeout_seconds = 5;
    int request_timeout_seconds = 180;
    std::string system_prompt;
};

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

Config load_config(const std::filesystem::path& path);

}  // namespace model_chat
