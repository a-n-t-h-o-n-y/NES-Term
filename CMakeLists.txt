cmake_minimum_required(VERSION 3.9)
project(gbemu-term LANGUAGES CXX)

add_subdirectory(external/TermOx)
add_subdirectory(external/SimpleNES)
add_subdirectory(src)
