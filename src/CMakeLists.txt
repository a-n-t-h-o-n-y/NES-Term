cmake_minimum_required(VERSION 3.9)

add_executable(laines-term main.cpp)

target_link_libraries(laines-term
    PRIVATE
        TermOx
        laines-lib
)

target_compile_features(laines-term
    PUBLIC
        cxx_std_17
)

target_compile_options(laines-term
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
