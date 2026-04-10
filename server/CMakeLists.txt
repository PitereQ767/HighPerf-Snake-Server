find_package(Threads REQUIRED)

add_executable(snake-server src/main.cpp)

target_include_directories(snake-server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(snake-server PRIVATE SharedProtocol Threads::Threads)