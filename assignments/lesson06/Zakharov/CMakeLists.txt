cmake_minimum_required(VERSION 3.20)

project(local_model_chat LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    cpp_httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(cpp_httplib)

add_library(model_chat_core
    src/chat.cpp
    src/config.cpp
    src/http_json_client.cpp
)
target_include_directories(model_chat_core PUBLIC include)
target_compile_features(model_chat_core PUBLIC cxx_std_17)
set_target_properties(model_chat_core PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(model_chat_core
    PUBLIC nlohmann_json::nlohmann_json
    PRIVATE httplib::httplib
)

add_executable(model_chat src/main.cpp)
target_link_libraries(model_chat PRIVATE model_chat_core)
if(MSVC)
    target_compile_options(model_chat_core PUBLIC /EHsc)
    target_compile_options(model_chat PRIVATE /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(model_chat_test tests/model_chat_test.cpp)
    target_link_libraries(model_chat_test PRIVATE model_chat_core)
    add_test(NAME model_chat_test COMMAND model_chat_test)
endif()
