cmake_minimum_required(VERSION 3.26)
project(codespy CXX)

find_package(miniz REQUIRED CONFIG)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(codespy)
add_subdirectory(sources)
target_compile_features(codespy PRIVATE cxx_std_20)
target_include_directories(codespy PUBLIC include)
target_link_libraries(codespy PRIVATE miniz::miniz Qt5::Widgets)

qt5_wrap_cpp(MOC_SOURCES
    include/codespy/gui/IrHighlighter.hh
    include/codespy/gui/MainWindow.hh
    include/codespy/gui/TextEdit.hh
    include/codespy/gui/TreeModel.hh)
target_sources(codespy PRIVATE ${MOC_SOURCES})
