cmake_minimum_required(VERSION 3.16)

set(TARGET_NAME better-frameless-window)
project(${TARGET_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


# If QT_DIR is defined, add it to CMAKE_PREFIX_PATH so find_package can locate Qt
if(DEFINED QT_DIR)
    list(APPEND CMAKE_PREFIX_PATH ${QT_DIR}) 
endif()

# Find Qt6 package, requiring Core, Gui and Widgets modules
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets
)

# Add executable and specify source files
add_executable(${TARGET_NAME})

set(BASE_SOURCES
    src/main.cpp
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

# Add sources to the target
target_sources(${TARGET_NAME} PRIVATE ${BASE_SOURCES})
if(WIN32)
    target_sources(${TARGET_NAME} PRIVATE ${WIN32_SOURCES})
endif()

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
)

# Link Qt6 libraries to the executable
target_link_libraries(${TARGET_NAME} PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets
)

# Link platform-specific libraries
if(WIN32)
    target_link_libraries(${TARGET_NAME} PRIVATE 
        dwmapi
        user32
    )
endif()   

# Deploy Qt dependencies using windeployqt (Windows) or macdeployqt (macOS)
if(WIN32)
    # Find windeployqt tool based on Qt6::qmake location
    if(TARGET Qt6::qmake)
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    else()
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/bin)
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        # Run windeployqt after build to copy necessary DLLs to the output directory
        # Use generator expressions to select --debug or --release based on configuration
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt dependencies via windeployqt..."
            COMMAND ${WINDEPLOYQT_EXECUTABLE} 
                $<$<CONFIG:Debug>:--debug>
                $<$<CONFIG:Release>:--release>
                --no-compiler-runtime # Avoid copying compiler runtime, only copy Qt libraries
                --no-opengl-sw        # Usually software rasterizer is not needed
                --no-translations     # Translation files are usually not needed during development
                --no-system-d3d-compiler
                --verbose 0
                $<TARGET_FILE:${TARGET_NAME}>
            COMMENT "Running windeployqt to copy DLLs..."
        )
    endif()
elseif(APPLE)
    # macOS Bundle configuration
    set_target_properties(${TARGET_NAME} PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
        MACOSX_BUNDLE_BUNDLE_NAME ${TARGET_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
    
    # Find macdeployqt tool (optional, for generating standalone .app)
    if(TARGET Qt6::qmake)
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
    else()
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${Qt6_DIR}/bin)
    endif()

    if(MACDEPLOYQT_EXECUTABLE)
        # Run macdeployqt after build to create .app bundle and copy necessary Frameworks
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${MACDEPLOYQT_EXECUTABLE} 
                $<TARGET_BUNDLE_DIR:${TARGET_NAME}>
                -verbose=0
            COMMENT "Running macdeployqt to bundle Frameworks..."
        )
    endif()
elseif(UNIX)
    # On Linux, extra deployment steps are usually not needed as Qt libraries should be installed on the system
    # However, you can add post-build steps if you need to copy specific resources or libraries
endif()

