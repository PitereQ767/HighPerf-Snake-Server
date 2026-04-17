find_package(SFML 2.6 COMPONENTS system window graphics REQUIRED)

include(FetchContent)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG v2.6.1
)

set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(imgui-sfml)

add_executable(snake-client
        src/main.cpp
        src/NetworkClient.cpp
        src/GameClient.cpp)

target_include_directories(snake-client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(snake-client PRIVATE
        SharedProtocol
        sfml-graphics
        sfml-window
        sfml-system
        ImGui-SFML::ImGui-SFML
)

target_compile_definitions(snake-client PRIVATE ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")