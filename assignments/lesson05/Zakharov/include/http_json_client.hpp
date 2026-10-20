#pragma once

#include "config.hpp"

#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace model_chat {

class HttpError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class HttpJsonClient {
public:
    explicit HttpJsonClient(const Config& config);

    nlohmann::json post(const std::string& endpoint,
                        const nlohmann::json& request) const;

private:
    std::string server_url_;
    int connect_timeout_seconds_;
    int request_timeout_seconds_;
};

}  // namespace model_chat
