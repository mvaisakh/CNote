cmake_minimum_required(VERSION 3.20)
project(CeriumNotes LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MUPDF REQUIRED mupdf)

qt_add_executable(CNotes
    src/main.cpp
    src/pdf/PdfEngine.cpp
    src/ink/InkProcessor.cpp
)

qt_add_qml_module(CNotes
    URI CeriumNotes
    VERSION 1.0
    QML_FILES
        qml/main.qml
)

target_include_directories(CNotes PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${MUPDF_INCLUDE_DIRS}
)

target_link_libraries(CNotes PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    ${MUPDF_LIBRARIES}
)

add_executable(TestInk
    src/ink/test_ink.cpp
    src/ink/InkProcessor.cpp
)
target_link_libraries(TestInk PRIVATE Qt6::Gui)
target_include_directories(TestInk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Optimization flags placeholder
# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#     target_compile_options(CeriumNotes PRIVATE -O3 -flto)
#     target_link_options(CeriumNotes PRIVATE -flto)
# endif()
