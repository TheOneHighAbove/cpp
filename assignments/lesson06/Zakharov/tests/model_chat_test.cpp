#include "chat.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

nlohmann::json reply_json(const std::string& text) {
    return {
        {"choices", nlohmann::json::array({
            {{"message", {{"role", "assistant"}, {"content", text}}}}
        })},
        {"usage", {
            {"prompt_tokens", 20},
            {"completion_tokens", 5},
            {"total_tokens", 25}
        }}
    };
}

void test_request_and_response() {
    model_chat::Config config;
    config.model = "local-model";
    const std::vector<model_chat::Message> messages{
        {"system", "Be concise"}, {"user", "Hello"}
    };
    const auto request = model_chat::build_request(config, messages);
    require(request.at("model") == "local-model", "wrong model");
    require(request.at("messages").size() == 2, "wrong message count");
    require(request.at("messages").at(1).at("content") == "Hello",
            "wrong user content");

    const auto reply = model_chat::parse_reply(reply_json("Hi"));
    require(reply.text == "Hi", "wrong reply text");
    require(reply.usage.total_tokens == 25, "wrong usage");
}

void test_history_trimming() {
    model_chat::Config config;
    config.context_size = 100;
    config.max_tokens = 20;
    config.system_prompt = "system";
    int count_calls = 0;

    model_chat::ChatSession session(
        config,
        [&count_calls](const std::string& endpoint, const nlohmann::json&) {
            if (endpoint.find("input_tokens") != std::string::npos) {
                ++count_calls;
                return nlohmann::json{{"input_tokens",
                    count_calls == 2 ? 90 : 30}};
            }
            return reply_json("answer");
        });

    const auto first = session.send("first");
    require(first.removed_pairs == 0, "first exchange trimmed history");
    const auto second = session.send("second");
    require(second.removed_pairs == 1, "oldest pair was not removed");
    require(session.messages().size() == 3, "unexpected history size");
    require(session.messages().at(1).content == "second",
            "current user message was removed");
}

void test_oversized_single_message_is_rolled_back() {
    model_chat::Config config;
    config.context_size = 100;
    config.max_tokens = 20;
    config.system_prompt = "system";
    model_chat::ChatSession session(
        config,
        [](const std::string&, const nlohmann::json&) {
            return nlohmann::json{{"input_tokens", 90}};
        });

    bool thrown = false;
    try {
        session.send("too long");
    } catch (const model_chat::ResponseError&) {
        thrown = true;
    }
    require(thrown, "oversized message must fail");
    require(session.messages().size() == 1,
            "failed current message remained in history");
}

void test_invalid_response() {
    bool thrown = false;
    try {
        model_chat::parse_reply({{"choices", nlohmann::json::array()}});
    } catch (const model_chat::ResponseError&) {
        thrown = true;
    }
    require(thrown, "invalid response must fail");
}

}  // namespace

int main() {
    try {
        test_request_and_response();
        test_history_trimming();
        test_oversized_single_message_is_rolled_back();
        test_invalid_response();
        std::cout << "All tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
