#include "http_json_client.hpp"

#include <httplib.h>

namespace model_chat {

HttpJsonClient::HttpJsonClient(const Config& config)
    : server_url_(config.server_url),
      connect_timeout_seconds_(config.connect_timeout_seconds),
      request_timeout_seconds_(config.request_timeout_seconds) {}

nlohmann::json HttpJsonClient::post(const std::string& endpoint,
                                    const nlohmann::json& request) const {
    httplib::Client client(server_url_);
    client.set_connection_timeout(connect_timeout_seconds_, 0);
    client.set_read_timeout(request_timeout_seconds_, 0);
    client.set_write_timeout(request_timeout_seconds_, 0);

    const auto result = client.Post(endpoint, request.dump(), "application/json");
    if (!result) {
        const std::string detail = httplib::to_string(result.error());
        if (result.error() == httplib::Error::ConnectionTimeout ||
            result.error() == httplib::Error::Read) {
            throw HttpError("request timed out after " +
                            std::to_string(request_timeout_seconds_) +
                            " seconds (" + detail + ")");
        }
        throw HttpError("cannot connect to llama-server at " + server_url_ +
                        " (" + detail + ")");
    }
    if (result->status < 200 || result->status >= 300) {
        throw HttpError("llama-server returned HTTP " +
                        std::to_string(result->status) + ": " + result->body);
    }

    try {
        return nlohmann::json::parse(result->body);
    } catch (const nlohmann::json::parse_error&) {
        throw HttpError("llama-server returned invalid JSON");
    }
}

}  // namespace model_chat
