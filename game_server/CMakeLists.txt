cmake_minimum_required(VERSION 3.25)
project(game_server)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(game_server
        server.cpp
        server.h
        main.cpp
        card.cpp
        card.h
        deck.cpp
        deck.h
        json.cpp
        json.h
        player.h
        lobby.cpp
        lobby.h
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(game_server PRIVATE _WIN32_WINNT=0x0A00) # Target Windows 10
    target_link_options(game_server PRIVATE -mconsole) # Specify console subsystem
    target_link_libraries(game_server PRIVATE ws2_32) # Winsock library
else()
    target_link_libraries(game_server PRIVATE Threads::Threads) # POSIX threading
endif()

target_link_libraries(game_server PRIVATE nlohmann_json::nlohmann_json)