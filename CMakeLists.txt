cmake_minimum_required(VERSION 3.25)
project(mclaren)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(third-party)

set(PLATFORM_SRCS platform.cc)
set(APP_SRCS app.cc camera.cc)
set(CORE_SRCS core/logging.cc core/deletion_queue.cc core/frame_graph.cc mesh_buffer.cc
        mesh_loader.cc
        event_system.cc
        input_system.cc
)
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
        vk_buffer.cc
        vk_queue.cc
        vk_descriptor.cc
        vk_descriptor_allocator.cc
        vk_descriptor_writer.cc
        vk_pipeline.cc
        vk_allocator.cc
)

add_executable(mclaren main.cc ${PLATFORM_SRCS} ${APP_SRCS} ${CORE_SRCS} ${VK_SRCS})
target_include_directories(mclaren PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mclaren PRIVATE volk SDL3-static VulkanMemoryAllocator imgui glm cgltf microprofile)
target_compile_definitions(mclaren PRIVATE VK_NO_PROTOTYPES)

if (IOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_IOS)
elseif (APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_MACOS)
elseif (__ANDROID__)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_ANDROID)
elseif (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
endif ()
