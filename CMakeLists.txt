cmake_minimum_required(VERSION 3.2)
project(PixelAnnotationTool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/ui)
set(CMAKE_PREFIX_PATH "D:/Qt/6.8.2/mingw_64")
set(OpenCV_DIR "D:/opencv4.8.1/build-qt-mingw/install")

configure_file(
        git_version.cmake.in
        ${CMAKE_BINARY_DIR}/git_version.cmake
)
include(${CMAKE_BINARY_DIR}/git_version.cmake)

find_package(
        Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED
)
find_package(OpenCV REQUIRED)

include_directories(
        ${OpenCV_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}
        include
)

file(GLOB_RECURSE CPP_INCLUDE ${PROJECT_SOURCE_DIR}/include/*.h)

file(GLOB_RECURSE CPP_SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)

file(GLOB_RECURSE UI_SRC ${PROJECT_SOURCE_DIR}/ui/*.ui)

add_executable(
        ${PROJECT_NAME}
        #        WIN32
        main.cpp
        ${CPP_INCLUDE}
        ${CPP_SRC}
        ${UI_SRC}
)

target_link_libraries(
        ${PROJECT_NAME}
        Qt::Core
        Qt::Gui
        Qt::Widgets
        ${OpenCV_LIBS}
)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB Core Gui Widgets)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()