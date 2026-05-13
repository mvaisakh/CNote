cmake_minimum_required(VERSION 3.20)

set(QT_ANDROID_MIN_SDK_VERSION 29)
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
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
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
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

option(ENABLE_PDF "Enable PDF support via MuPDF" ON)

# MuPDF Dependency Handling
if(ENABLE_PDF)
    set(MUPDF_FOUND FALSE)
    
    # 1. ALWAYS check the vendored third_party folder first (Crucial for CI)
    find_path(MUPDF_INCLUDE_DIR mupdf/fitz.h
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/include
        NO_DEFAULT_PATH)
        
    if(MUPDF_INCLUDE_DIR)
        set(MUPDF_INCLUDE_DIRS ${MUPDF_INCLUDE_DIR})
        if(WIN32)
            file(GLOB MUPDF_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/lib/*.lib")
        else()
            file(GLOB MUPDF_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/lib/*.a")
        endif()
        
        if(MUPDF_LIBRARIES)
            set(MUPDF_FOUND TRUE)
            message(STATUS "MuPDF found locally in third_party")
            message(STATUS "MuPDF Include Dir: ${MUPDF_INCLUDE_DIR}")
            message(STATUS "MuPDF Libraries: ${MUPDF_LIBRARIES}")
        endif()
    endif()

    # 2. Fallback to PkgConfig for Linux Desktop (if not found in third_party)
    if(NOT MUPDF_FOUND AND NOT ANDROID AND NOT IOS AND NOT WIN32)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(MUPDF mupdf IMPORTED_TARGET)
            if(MUPDF_FOUND)
                message(STATUS "MuPDF found via pkg-config")
                set(MUPDF_LIBRARIES PkgConfig::MUPDF)
                set(MUPDF_INCLUDE_DIRS ${MUPDF_INCLUDE_DIRS}) # Handled by pkg-config
            endif()
        endif()
    endif()

    # 3. Apply the results
    if(MUPDF_FOUND)
        add_definitions(-DENABLE_PDF)
        message(STATUS "PDF support enabled")
        message(STATUS "Final MuPDF Include Dirs: ${MUPDF_INCLUDE_DIRS}")
        message(STATUS "Final MuPDF Libraries: ${MUPDF_LIBRARIES}")
    else()
        message(STATUS "MuPDF NOT FOUND!")
        message(STATUS "Search path: ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/include")
        message(FATAL_ERROR "MuPDF not found! Cannot compile PDF engine.")
        # We use FATAL_ERROR because your C++ code explicitly #includes fitz.h. 
        # If it's missing, the build will fail anyway, so it's better to fail fast in CMake.
    endif()
endif()

qt_add_executable(CNotes
    MACOSX_BUNDLE
    WIN32
    src/main.cpp
    src/pdf/PdfEngine.cpp
    src/ink/InkProcessor.cpp
    src/core/CrashHandler.cpp
)

set_target_properties(CNotes PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    OUTPUT_NAME "CeriumNotes"
)

if(NOT WIN32)
    target_link_options(CNotes PRIVATE -rdynamic)
endif()

qt_add_qml_module(CNotes
    URI CNote.Core
    VERSION 1.0
    RESOURCE_PREFIX /
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

if(IOS)
    set_target_properties(CNotes PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.cerium.notes"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"

        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
    )
endif()

target_compile_definitions(CNotes PRIVATE 
    APP_VERSION_STR="${APP_VERSION_STR}"
)

target_include_directories(CNotes PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render
    ${MUPDF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mupdf/include
)

target_link_libraries(CNotes PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

target_link_libraries(CNotes PRIVATE ${MUPDF_LIBRARIES})

if(NOT ANDROID AND NOT IOS)
    add_executable(TestInk
        src/ink/test_ink.cpp
        src/ink/InkProcessor.cpp
    )
    target_link_libraries(TestInk PRIVATE Qt6::Gui)
    target_include_directories(TestInk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif()

# Optimization flags placeholder
# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#     target_compile_options(CNotes PRIVATE -O3 -flto)
#     target_link_options(CNotes PRIVATE -flto)
# endif()
