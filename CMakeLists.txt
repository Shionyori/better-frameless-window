cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME better-frameless-window)
project(${TARGET_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(DEFINED QT_DIR)
    list(APPEND CMAKE_PREFIX_PATH ${QT_DIR})
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

# Static library
add_library(${TARGET_NAME} STATIC)

set(LIB_SOURCES
    src/framelesswindow.cpp
    src/framelesswindow.h
    src/titlebar.cpp
    src/titlebar.h
    src/core/windowvisualstate.cpp
    src/core/windowvisualstate.h
    src/core/visualrefreshcoordinator.cpp
    src/core/visualrefreshcoordinator.h
    src/thememanager.cpp
    src/thememanager.h
    src/diagnostics.cpp
    src/diagnostics.h
)

set(WIN32_SOURCES
    src/platform/win32/nativemessagerouter.cpp
    src/platform/win32/nativemessagerouter.h
    src/platform/win32/systemmenu.cpp
    src/platform/win32/systemmenu.h
    src/platform/win32/windowcommand.cpp
    src/platform/win32/windowcommand.h
    src/platform/win32/windowframe.cpp
    src/platform/win32/windowframe.h
    src/platform/win32/windowhittest.cpp
    src/platform/win32/windowhittest.h
    src/platform/win32/windoweffect.cpp
    src/platform/win32/windoweffect.h
    src/platform/win32/utils.cpp
    src/platform/win32/utils.h
)

target_sources(${TARGET_NAME} PRIVATE ${LIB_SOURCES})
if(WIN32)
    target_sources(${TARGET_NAME} PRIVATE ${WIN32_SOURCES})
endif()

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
)

target_link_libraries(${TARGET_NAME} PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(WIN32)
    target_link_libraries(${TARGET_NAME} PUBLIC
        dwmapi
        user32
    )
endif()

# Example executable
add_executable(bfw-example example/main.cpp)
target_link_libraries(bfw-example PRIVATE ${TARGET_NAME})

# Qt deployment (Windows / macOS) 
function(deploy_qt target)
    if(WIN32)
        if(TARGET Qt6::qmake)
            get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
            get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
            find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
        else()
            find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/bin)
        endif()

        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt dependencies for ${target}..."
                COMMAND ${WINDEPLOYQT_EXECUTABLE}
                    $<$<CONFIG:Debug>:--debug>
                    $<$<CONFIG:Release>:--release>
                    --no-compiler-runtime
                    --no-opengl-sw
                    --no-translations
                    --no-system-d3d-compiler
                    --verbose 0
                    $<TARGET_FILE:${target}>
                COMMENT "Running windeployqt for ${target}..."
            )
        endif()
    elseif(APPLE)
        set_target_properties(${target} PROPERTIES
            MACOSX_BUNDLE ON
            MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
            MACOSX_BUNDLE_BUNDLE_NAME ${target}
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        )

        if(TARGET Qt6::qmake)
            get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
            get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
            find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
        else()
            find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${Qt6_DIR}/bin)
        endif()

        if(MACDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${MACDEPLOYQT_EXECUTABLE}
                    $<TARGET_BUNDLE_DIR:${target}>
                    -verbose=0
                COMMENT "Running macdeployqt for ${target}..."
            )
        endif()
    endif()
endfunction()

deploy_qt(bfw-example)
