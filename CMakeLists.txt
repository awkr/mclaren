cmake_minimum_required(VERSION 3.26)
project(mclaren)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(third-party)

set(PLATFORM_SRCS platform.cc)
set(APP_SRCS app.cc)
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
    vk_descriptor.cc
    vk_pipeline.cc
    vk_allocator.cc
)

add_executable(mclaren main.cc ${PLATFORM_SRCS} ${APP_SRCS} ${CORE_SRCS} ${VK_SRCS})
target_link_libraries(mclaren PRIVATE volk SDL3-static VulkanMemoryAllocator imgui)
target_compile_definitions(mclaren PRIVATE VK_NO_PROTOTYPES)
