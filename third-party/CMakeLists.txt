add_subdirectory(volk)

set(SDL_STATIC ON CACHE BOOL "")
set(SDL_SHARED OFF CACHE BOOL "")
set(SDL_TEST_LIBRARY ON CACHE BOOL "")
set(SDL_TESTS ON CACHE BOOL "")
set(SDL_DISABLE_UNINSTALL ON CACHE BOOL "")
set(SDL_ASSEMBLY OFF CACHE BOOL "")
set(SDL_DUMMYVIDEO OFF CACHE BOOL "")
set(SDL_METAL OFF CACHE BOOL "")
set(SDL_OPENGL OFF CACHE BOOL "")
set(SDL_OPENGLES OFF CACHE BOOL "")
# set(SDL_HIDAPI OFF CACHE BOOL "")
set(SDL_HIDAPI_JOYSTICK OFF CACHE BOOL "")
set(SDL_HIDAPI_LIBUSB OFF CACHE BOOL "")
set(SDL_HIDAPI_LIBUSB_SHARED OFF CACHE BOOL "")
set(SDL_VIRTUAL_JOYSTICK OFF CACHE BOOL "")
set(SDL_AUDIO OFF CACHE BOOL "")
set(SDL_RENDER OFF CACHE BOOL "")
set(SDL_CAMERA OFF CACHE BOOL "")
# set(SDL_JOYSTICK OFF CACHE BOOL "")
set(SDL_HAPTIC OFF CACHE BOOL "")
set(SDL_POWER OFF CACHE BOOL "")
set(SDL_SENSOR OFF CACHE BOOL "")
set(SDL_DIALOG_DUMMY OFF CACHE BOOL "")
add_subdirectory(SDL)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h")
add_library(imgui STATIC ${IMGUI_FILES})
target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR})

add_subdirectory(glm)
add_subdirectory(cgltf)
add_subdirectory(microprofile)
add_subdirectory(stb)
add_subdirectory(meshoptimizer)
