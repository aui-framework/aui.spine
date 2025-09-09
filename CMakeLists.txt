# Standard routine
cmake_minimum_required(VERSION 3.16)
project(aui.spine
        VERSION 0.0.10 # CI_PROJECT_VERSION
    )

option(BUILD_SHARED_LIBS "Build shared libs" OFF)
option(AUI_SPINE_EXAMPLES "Build examples for aui.spine" OFF)
option(SPINE_SET_COMPILER_FLAGS "Set compiler flags" OFF) # -fno-rtti breaks linking
set(AUI_VERSION 2c54fe7)
set(AUI_SPINE_VERSION "e75e976" # 4.2
        CACHE STRING "Spine version used in aui.spine")

# Use AUI.Boot
file(
        DOWNLOAD
        https://raw.githubusercontent.com/aui-framework/aui/${AUI_VERSION}/aui.boot.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/aui.boot.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/aui.boot.cmake)

# import AUI
auib_import(aui https://github.com/aui-framework/aui
        COMPONENTS core views
        VERSION ${AUI_VERSION})

auib_import(spine https://github.com/aui-framework/spine-runtimes
            VERSION ${AUI_SPINE_VERSION}
            CMAKE_WORKING_DIR spine-cpp
)

# Create the executable. This function automatically links all sources from the src/ folder,
# creates CMake target and places the resulting executable to bin/ folder.
aui_module(${PROJECT_NAME} EXPORT spine)

# Compile auisl shaders.
auisl_shader(${PROJECT_NAME} spine.vsh)
auisl_shader(${PROJECT_NAME} spine.fsh)

# Link required libs
aui_link(${PROJECT_NAME} PUBLIC aui::core aui::views spine::spine-cpp)

if (AUI_SPINE_EXAMPLES)
    add_subdirectory(example)
endif ()

auib_precompiled_binary()

if (NOT AUIB_AUI_AS)
    # Install target
    install(TARGETS ${PROJECT_NAME} EXPORT aui.spine DESTINATION lib)
    install(EXPORT aui.spine DESTINATION "lib/cmake"
            FILE aui.spine-targets.cmake)

    include(CMakePackageConfigHelpers)
    # generate the config file that includes the exports
    configure_package_config_file(aui.spine-config.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/aui.spine-config.cmake"
            INSTALL_DESTINATION "."
            NO_SET_AND_CHECK_MACRO
            NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/aui.spine-config.cmake
            DESTINATION "."
    )
endif ()
