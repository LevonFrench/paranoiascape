cmake_minimum_required(VERSION 3.20)
project(PSXRecompGame CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GLFW
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLAD — pre-generated into lib/glad (avoids Python discovery issues)
add_library(glad STATIC lib/glad/src/glad.c)
target_include_directories(glad PUBLIC lib/glad/include)

# fmt
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.1.1
)
FetchContent_MakeAvailable(fmt)

add_executable(recomp
    src/main_runner.cpp
    src/runtime.c
    src/gte.cpp
    src/gpu_state.cpp
    src/gpu_interpreter.cpp
    src/opengl_renderer.cpp
    src/ps1_exe_parser.cpp
    src/overlay_detector.cpp
    src/overlay_manager.cpp
    src/iso_reader.cpp
    src/cdrom_stub.cpp
    src/xa_audio.cpp
    src/cd_audio.cpp
    src/fmv_player.cpp
    src/spu.cpp
    src/input_script.cpp
    src/extras.cpp
    src/launcher.cpp
    src/debug_server.c
    src/func_logger.c
    src/crc32.c
    src/savestate.cpp
    src/interp_stub.c
    # Generated — never edit these directly:
    ../generated/SLPS_013.75_full.c
    ../generated/SLPS_013.75_dispatch.c
)

target_include_directories(recomp PRIVATE
    include
    ../generated
)

target_link_libraries(recomp PRIVATE glfw glad fmt)

if(WIN32)
    target_link_libraries(recomp PRIVATE opengl32 winmm ws2_32)
endif()

# Note: Place title screen MP3 at:
#   <build_output_dir>/audio/title_screen.mp3
# The game will auto-play it when the main menu is reached (~frame 750).
# Audio will loop until the game exits.

# Copy assets to the output directory so the exe can find them at runtime
add_custom_command(TARGET recomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/keyconfig.ini"
        "$<TARGET_FILE_DIR:recomp>/keyconfig.ini"
    COMMENT "Copying keyconfig.ini to output directory"
)
