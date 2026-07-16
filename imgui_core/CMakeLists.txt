# CTF|认证
cmake_minimum_required(VERSION 3.22)
project(delta_imgui_core LANGUAGES CXX)
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui" CACHE PATH "Dear ImGui source")
add_library(delta_imgui_core STATIC
    imgui_panel.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp)
target_include_directories(delta_imgui_core PUBLIC . ../android_client ${IMGUI_DIR})
target_compile_features(delta_imgui_core PUBLIC cxx_std_17)
