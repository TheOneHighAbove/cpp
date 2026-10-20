#include "chat.hpp"
#include "config.hpp"
#include "http_json_client.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc > 2) {
        std::cerr << "Usage: model_chat [config.json]\n";
        return 1;
    }

    try {
        const std::filesystem::path config_path =
            argc == 2 ? argv[1] : "config.example.json";
        const auto config = model_chat::load_config(config_path);
        const model_chat::HttpJsonClient http(config);
        model_chat::ChatSession session(
            config, [&http](const std::string& endpoint,
                            const nlohmann::json& request) {
                return http.post(endpoint, request);
            });

        std::cerr << "Local chat is ready. Commands: /reset, /exit\n";
        while (true) {
            std::cerr << "You: ";
            std::string text;
            if (!std::getline(std::cin, text)) {
                std::cerr << '\n';
                break;
            }
            if (text == "/exit") {
                break;
            }
            if (text == "/reset") {
                session.reset();
                std::cout << "History cleared.\n";
                continue;
            }

            const auto result = session.send(text);
            if (result.removed_pairs > 0) {
                std::cerr << "History shortened: removed "
                          << result.removed_pairs << " oldest pair(s).\n";
            }
            std::cout << "Assistant: " << result.reply.text << '\n';
            std::cout << "Tokens: input " << result.reply.usage.prompt_tokens
                      << ", answer " << result.reply.usage.completion_tokens
                      << ", total " << result.reply.usage.total_tokens
                      << " / " << config.context_size << "\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
