cmake_minimum_required(VERSION 3.9)

add_executable(nes-term main.cpp)

target_link_libraries(nes-term
    PRIVATE
        TermOx
        SimpleNES
)

target_compile_features(nes-term
    PUBLIC
        cxx_std_17
)

target_compile_options(nes-term
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
