#include "config.hpp"

#include <cmath>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace model_chat {
namespace {

using nlohmann::json;

const json& require_field(const json& value, const char* name) {
    const auto iterator = value.find(name);
    if (iterator == value.end()) {
        throw ConfigError("missing required field: " + std::string(name));
    }
    return *iterator;
}

std::string require_non_empty_string(const json& value, const char* name) {
    const auto& field = require_field(value, name);
    if (!field.is_string() || field.get<std::string>().empty()) {
        throw ConfigError("field '" + std::string(name) +
                          "' must be a non-empty string");
    }
    return field.get<std::string>();
}

int require_positive_integer(const json& value, const char* name) {
    const auto& field = require_field(value, name);
    if (!field.is_number_integer() && !field.is_number_unsigned()) {
        throw ConfigError("field '" + std::string(name) +
                          "' must be a positive integer no greater than 86400");
    }
    const bool in_range = field.is_number_unsigned()
        ? field.get<unsigned long long>() > 0 &&
              field.get<unsigned long long>() <= 86400
        : field.get<long long>() > 0 && field.get<long long>() <= 86400;
    if (!in_range) {
        throw ConfigError("field '" + std::string(name) +
                          "' must be a positive integer no greater than 86400");
    }
    return field.get<int>();
}

}  // namespace

Config load_config(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw ConfigError("cannot open configuration file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad()) {
        throw ConfigError("cannot read configuration file: " + path.string());
    }

    json parsed;
    try {
        parsed = json::parse(buffer.str());
    } catch (const json::parse_error& error) {
        throw ConfigError("invalid JSON syntax: " + std::string(error.what()));
    }
    if (!parsed.is_object()) {
        throw ConfigError("configuration root must be a JSON object");
    }

    Config config;
    config.server_url = require_non_empty_string(parsed, "server_url");
    if (config.server_url.rfind("http://", 0) != 0) {
        throw ConfigError("field 'server_url' must start with http://");
    }
    while (config.server_url.size() > 7 && config.server_url.back() == '/') {
        config.server_url.pop_back();
    }
    config.model = require_non_empty_string(parsed, "model");
    config.context_size = require_positive_integer(parsed, "context_size");
    config.max_tokens = require_positive_integer(parsed, "max_tokens");
    if (config.max_tokens >= config.context_size) {
        throw ConfigError("max_tokens must be smaller than context_size");
    }

    const auto& temperature = require_field(parsed, "temperature");
    if (!temperature.is_number()) {
        throw ConfigError("field 'temperature' must be a number");
    }
    config.temperature = temperature.get<double>();
    if (!std::isfinite(config.temperature) || config.temperature < 0.0 ||
        config.temperature > 2.0) {
        throw ConfigError("field 'temperature' must be in range [0, 2]");
    }

    config.connect_timeout_seconds =
        require_positive_integer(parsed, "connect_timeout_seconds");
    config.request_timeout_seconds =
        require_positive_integer(parsed, "request_timeout_seconds");

    const auto& system_prompt = require_field(parsed, "system_prompt");
    if (!system_prompt.is_string()) {
        throw ConfigError("field 'system_prompt' must be a string");
    }
    config.system_prompt = system_prompt.get<std::string>();
    return config;
}

}  // namespace model_chat
