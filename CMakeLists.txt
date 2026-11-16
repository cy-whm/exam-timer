cmake_minimum_required(VERSION 3.20)
project(ExamTimer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ExamTimer WIN32
    src/main.cpp
    src/app.cpp
    src/render.cpp
    src/ui.cpp
    res/app.rc
)

target_compile_definitions(ExamTimer PRIVATE
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN NOMINMAX
    _WIN32_WINNT=0x0A00 WINVER=0x0A00
)

target_compile_options(ExamTimer PRIVATE /W4 /permissive- /utf-8 /MP /EHsc)

target_link_libraries(ExamTimer PRIVATE
    d2d1 dwrite dwmapi shell32 winmm user32 gdi32 ole32
)

set_target_properties(ExamTimer PROPERTIES OUTPUT_NAME "Timer")

# 清单由 res/app.rc 内嵌，禁止链接器再生成一份
target_link_options(ExamTimer PRIVATE /MANIFEST:NO)

# 静态链接运行库，单文件分发
set_property(TARGET ExamTimer PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
