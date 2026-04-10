add_executable(snake-client src/main.cpp)

target_include_directories(snake-client PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(snake-client PRIVATE SharedProtocol)