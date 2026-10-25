#pragma once

#include "config.hpp"

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace model_chat {

struct Message {
    std::string role;
    std::string content;
};

struct Usage {
    int prompt_tokens;
    int completion_tokens;
    int total_tokens;
};

struct Reply {
    std::string text;
    Usage usage;
};

struct ExchangeResult {
    Reply reply;
    int removed_pairs;
};

class ResponseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

using PostJson = std::function<nlohmann::json(
    const std::string&, const nlohmann::json&)>;

nlohmann::json build_request(const Config& config,
                             const std::vector<Message>& messages);
int parse_input_tokens(const nlohmann::json& response);
Reply parse_reply(const nlohmann::json& response);

class ChatSession {
public:
    ChatSession(Config config, PostJson post_json);

    ExchangeResult send(const std::string& user_text);
    void reset();

    const std::vector<Message>& messages() const noexcept;

private:
    bool remove_oldest_pair();

    Config config_;
    PostJson post_json_;
    std::vector<Message> messages_;
};

}  // namespace model_chat
