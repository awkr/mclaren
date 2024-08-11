cmake_minimum_required(VERSION 3.26)
project(mclaren)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(third-party/volk)
add_subdirectory(third-party/VulkanMemoryAllocator)

set(SDL_STATIC ON CACHE BOOL "")
set(SDL_SHARED OFF CACHE BOOL "")
set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
set(SDL_DISABLE_UNINSTALL ON CACHE BOOL "")
set(SDL_ASSEMBLY OFF CACHE BOOL "")
set(SDL_DUMMYVIDEO OFF CACHE BOOL "")
set(SDL_METAL OFF CACHE BOOL "")
set(SDL_OPENGL OFF CACHE BOOL "")
set(SDL_OPENGLES OFF CACHE BOOL "")
#set(SDL_HIDAPI OFF CACHE BOOL "")
set(SDL_HIDAPI_JOYSTICK OFF CACHE BOOL "")
set(SDL_HIDAPI_LIBUSB OFF CACHE BOOL "")
set(SDL_HIDAPI_LIBUSB_SHARED OFF CACHE BOOL "")
set(SDL_VIRTUAL_JOYSTICK OFF CACHE BOOL "")
set(SDL_AUDIO OFF CACHE BOOL "")
set(SDL_RENDER OFF CACHE BOOL "")
set(SDL_CAMERA OFF CACHE BOOL "")
#set(SDL_JOYSTICK OFF CACHE BOOL "")
set(SDL_HAPTIC OFF CACHE BOOL "")
set(SDL_POWER OFF CACHE BOOL "")
set(SDL_SENSOR OFF CACHE BOOL "")
set(SDL_DIALOG OFF CACHE BOOL "")
add_subdirectory(third-party/SDL)

set(PLATFORM_SRCS platform.cc)
set(APP_SRCS application.cc)
set(CORE_SRCS logging.cc deletion_queue.cc)
set(VK_SRCS
    vk.cc
    vk_instance.cc
    vk_device.cc
    vk_swapchain.cc
    vk_command_pool.cc
    vk_command_buffer.cc
    vk_fence.cc
    vk_semaphore.cc
    vk_image.cc
    vk_image_view.cc
    vk_queue.cc
    vk_allocator.cc
)

add_executable(mclaren main.cc ${PLATFORM_SRCS} ${APP_SRCS} ${CORE_SRCS} ${VK_SRCS})
target_link_libraries(mclaren PRIVATE volk SDL3-static VulkanMemoryAllocator)
target_compile_definitions(mclaren PRIVATE VK_NO_PROTOTYPES)
