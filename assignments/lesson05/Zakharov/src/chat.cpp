#include "chat.hpp"

#include <limits>
#include <utility>

namespace model_chat {
namespace {

int require_non_negative_integer(const nlohmann::json& object,
                                 const char* field,
                                 const char* error_message) {
    if (!object.is_object() || !object.contains(field)) {
        throw ResponseError(error_message);
    }
    const auto& value = object.at(field);
    if (!value.is_number_integer() && !value.is_number_unsigned()) {
        throw ResponseError(error_message);
    }
    const bool in_range = value.is_number_unsigned()
        ? value.get<unsigned long long>() <=
              static_cast<unsigned long long>(std::numeric_limits<int>::max())
        : value.get<long long>() >= 0 &&
              value.get<long long>() <= std::numeric_limits<int>::max();
    if (!in_range) {
        throw ResponseError(error_message);
    }
    return value.get<int>();
}

}  // namespace

nlohmann::json build_request(const Config& config,
                             const std::vector<Message>& messages) {
    nlohmann::json message_array = nlohmann::json::array();
    for (const auto& message : messages) {
        message_array.push_back({
            {"role", message.role},
            {"content", message.content}
        });
    }

    return {
        {"model", config.model},
        {"messages", std::move(message_array)},
        {"temperature", config.temperature},
        {"max_tokens", config.max_tokens},
        {"stream", false}
    };
}

int parse_input_tokens(const nlohmann::json& response) {
    return require_non_negative_integer(
        response, "input_tokens",
        "token-count response has no non-negative integer 'input_tokens'");
}

Reply parse_reply(const nlohmann::json& response) {
    const char* content_error =
        "response has no string choices[0].message.content";
    if (!response.is_object() || !response.contains("choices") ||
        !response.at("choices").is_array() ||
        response.at("choices").empty() ||
        !response.at("choices").at(0).is_object()) {
        throw ResponseError(content_error);
    }
    const auto& choice = response.at("choices").at(0);
    if (!choice.contains("message") || !choice.at("message").is_object() ||
        !choice.at("message").contains("content") ||
        !choice.at("message").at("content").is_string()) {
        throw ResponseError(content_error);
    }
    if (!response.contains("usage") || !response.at("usage").is_object()) {
        throw ResponseError("response has no valid 'usage' object");
    }

    const auto& usage_json = response.at("usage");
    Usage usage{
        require_non_negative_integer(usage_json, "prompt_tokens",
                                     "usage.prompt_tokens is missing or invalid"),
        require_non_negative_integer(usage_json, "completion_tokens",
                                     "usage.completion_tokens is missing or invalid"),
        require_non_negative_integer(usage_json, "total_tokens",
                                     "usage.total_tokens is missing or invalid")
    };
    return {choice.at("message").at("content").get<std::string>(), usage};
}

ChatSession::ChatSession(Config config, PostJson post_json)
    : config_(std::move(config)), post_json_(std::move(post_json)) {
    if (!post_json_) {
        throw std::invalid_argument("post_json callback must be set");
    }
    reset();
}

ExchangeResult ChatSession::send(const std::string& user_text) {
    if (user_text.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument("message must not be empty");
    }

    messages_.push_back({"user", user_text});
    int removed_pairs = 0;
    try {
        auto request = build_request(config_, messages_);
        int input_tokens = parse_input_tokens(post_json_(
            "/v1/chat/completions/input_tokens", request));

        while (input_tokens + config_.max_tokens > config_.context_size) {
            if (!remove_oldest_pair()) {
                throw ResponseError(
                    "the current message does not fit the 1024-token context; "
                    "shorten it and try again");
            }
            ++removed_pairs;
            request = build_request(config_, messages_);
            input_tokens = parse_input_tokens(post_json_(
                "/v1/chat/completions/input_tokens", request));
        }

        const Reply reply = parse_reply(
            post_json_("/v1/chat/completions", request));
        messages_.push_back({"assistant", reply.text});
        return {reply, removed_pairs};
    } catch (...) {
        if (!messages_.empty() && messages_.back().role == "user") {
            messages_.pop_back();
        }
        throw;
    }
}

void ChatSession::reset() {
    messages_.clear();
    if (!config_.system_prompt.empty()) {
        messages_.push_back({"system", config_.system_prompt});
    }
}

const std::vector<Message>& ChatSession::messages() const noexcept {
    return messages_;
}

bool ChatSession::remove_oldest_pair() {
    const std::size_t start =
        !messages_.empty() && messages_.front().role == "system" ? 1 : 0;
    for (std::size_t index = start; index + 1 < messages_.size(); ++index) {
        if (messages_[index].role == "user" &&
            messages_[index + 1].role == "assistant") {
            messages_.erase(messages_.begin() + static_cast<std::ptrdiff_t>(index),
                            messages_.begin() +
                                static_cast<std::ptrdiff_t>(index + 2));
            return true;
        }
    }
    return false;
}

}  // namespace model_chat
