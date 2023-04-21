cmake_minimum_required(VERSION 3.26)
project(codespy CXX)

find_package(miniz REQUIRED CONFIG)

add_executable(codespy)
add_subdirectory(sources)
target_compile_features(codespy PRIVATE cxx_std_20)
target_include_directories(codespy PUBLIC include)
target_link_libraries(codespy PRIVATE miniz::miniz)
