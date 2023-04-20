cmake_minimum_required(VERSION 3.26)
project(bytecode)

find_package(miniz REQUIRED CONFIG)

add_executable(bytecode sources/bytecode/ClassFile.cc sources/support/Stream.cc sources/support/String.cc sources/support/StringBuilder.cc sources/support/Print.cc sources/main.cc)
target_compile_features(bytecode PRIVATE cxx_std_20)
target_include_directories(bytecode PUBLIC include)
target_link_libraries(bytecode PRIVATE miniz::miniz)
