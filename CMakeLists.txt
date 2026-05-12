cmake_minimum_required(VERSION 3.20)

set(QT_ANDROID_MIN_SDK_VERSION 24)
set(QT_ANDROID_TARGET_SDK_VERSION 34)
set(QT_ANDROID_BUILD_TOOLS_REVISION 34.0.0)

project(CeriumNotes LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Versioning Logic
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GIT_STATUS)
        set(GIT_DIRTY "-dirty")
    endif()
endif()

if(NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()

if(ANDROID)
    set(PLATFORM "android")
elseif(APPLE)
    # Check if building for iOS or macOS
    if(IOS)
        set(PLATFORM "ios")
    else()
        set(PLATFORM "macos")
    endif()
elseif(WIN32)
    set(PLATFORM "windows")
else()
    set(PLATFORM "linux")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" BASE_VERSION)
string(STRIP "${BASE_VERSION}" BASE_VERSION)

cmake_host_system_information(RESULT HOSTNAME_RES QUERY HOSTNAME)
set(APP_VERSION_STR "${BASE_VERSION}-${PLATFORM}-${GIT_HASH}-${HOSTNAME_RES}${GIT_DIRTY}")
message(STATUS "Building CeriumNotes version: ${APP_VERSION_STR}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)
qt_standard_project_setup()

option(ENABLE_PDF "Enable PDF support via MuPDF" ON)

# Auto-disable PDF on platforms where dependencies are complex to fetch in CI
if(ANDROID OR IOS OR WIN32)
    if(NOT MUPDF_FOUND)
        set(ENABLE_PDF OFF)
        message(WARNING "MuPDF not found; PDF support disabled for this platform build.")
    endif()
endif()

# MuPDF Dependency Handling
if(ENABLE_PDF)
    set(MUPDF_FOUND FALSE)
    
    # 1. Check for vendored MuPDF in third_party
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf")
        set(MUPDF_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/include")
        set(MUPDF_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/lib/libmupdf.a") 
        set(MUPDF_FOUND TRUE)
    endif()

    # 2. Platform-specific fallbacks
    if(NOT MUPDF_FOUND AND NOT ANDROID)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(MUPDF mupdf)
        endif()
    endif()

    # Prioritize local third_party build for CI/CD consistency
    find_path(MUPDF_INCLUDE_DIR mupdf/fitz.h
        PATHS ${CMAKE_SOURCE_DIR}/third_party/mupdf/include
        NO_CACHE)
    
    if(MUPDF_INCLUDE_DIR)
        set(MUPDF_INCLUDE_DIRS ${MUPDF_INCLUDE_DIR})
        # Link all static libraries found in the local lib directory
        if(WIN32)
            file(GLOB MUPDF_LIBRARIES "${CMAKE_SOURCE_DIR}/third_party/mupdf/lib/*.lib")
        else()
            file(GLOB MUPDF_LIBRARIES "${CMAKE_SOURCE_DIR}/third_party/mupdf/lib/*.a")
        endif()
        
        if(MUPDF_LIBRARIES)
            set(MUPDF_FOUND TRUE)
            add_definitions(-DENABLE_PDF)
            message(STATUS "PDF support enabled (local build)")
        endif()
    endif()

    if(NOT MUPDF_FOUND)
        message(WARNING "MuPDF not found; PDF support disabled")
        set(ENABLE_PDF OFF)
    endif()
endif()

qt_add_executable(CNotes
    src/main.cpp
    src/pdf/PdfEngine.cpp
    src/ink/InkProcessor.cpp
    src/core/CrashHandler.cpp
)

set_target_properties(CNotes PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
)

target_link_options(CNotes PRIVATE -rdynamic)

qt_add_qml_module(CNotes
    URI CeriumNotes
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/Toolbar.qml
        qml/Dashboard.qml
    RESOURCES
        icons/pen.svg
        icons/highlighter.svg
        icons/eraser.svg
        icons/export.svg
        icons/back.svg
        icons/add.svg
    SOURCES
        src/render/NoteCanvas.h
        src/render/NoteCanvas.cpp
        src/core/FileManager.h
        src/core/FileManager.cpp
        src/core/PersistenceManager.h
        src/core/PersistenceManager.cpp
        src/pdf/ExportEngine.h
        src/pdf/ExportEngine.cpp
    DEPENDENCIES
        Quick
)

target_compile_definitions(CNotes PRIVATE 
    APP_VERSION_STR="${APP_VERSION_STR}"
)

target_include_directories(CNotes PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render
    ${MUPDF_INCLUDE_DIRS}
)

target_link_libraries(CNotes PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

target_link_libraries(CNotes PRIVATE ${MUPDF_LIBRARIES})

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
