project(aui.spine.example)

aui_executable(${PROJECT_NAME})
aui_link(${PROJECT_NAME} PRIVATE aui.spine)
aui_compile_assets(${PROJECT_NAME})

aui_app(TARGET ${PROJECT_NAME}
        NAME "AUI Spine"
        VERSION ${PROJECT_VERSION}
        ICON "assets/img/icon.svg")
