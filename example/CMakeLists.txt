project(aui.spine.example)

aui_executable(${PROJECT_NAME})

aui_link(${PROJECT_NAME} PRIVATE aui.spine)

# pull sample assets from spine official distribution.
file(COPY ${spine-cpp_SOURCE_DIR}/../spine-ts/spine-threejs/example/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/samples/anim)

# Compile assets
aui_compile_assets(${PROJECT_NAME} DIR ${CMAKE_CURRENT_BINARY_DIR}/samples)
