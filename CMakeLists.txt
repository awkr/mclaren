cmake_minimum_required(VERSION 3.20)
project(mclaren)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(third-party)

set(PLATFORM_SRCS platform.cc)
set(APP_SRCS app.cc camera.cc)
set(CORE_SRCS
    logging.cc
    core/deletion_queue.cc
    mesh_loader.cc
    mesh.cc
    geometry.cc
    raycast.cc
    event_system.cc
    input_system.cc
    mesh_system.cc
    geometry_system.cc
    gizmo.cc
    frame_graph.cc
    # scene.cc
    shadow_pass.cc
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
    vk_sampler.cc
    vk_buffer.cc
    vk_queue.cc
    vk_descriptor.cc
    vk_descriptor_allocator.cc
    vk_descriptor_writer.cc
    vk_pipeline.cc
    vk_render_pass.cc
    vk_framebuffer.cc
)

add_executable(mclaren main.cc ${PLATFORM_SRCS} ${APP_SRCS} ${CORE_SRCS} ${VK_SRCS})
target_include_directories(mclaren PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mclaren PRIVATE volk SDL3-static imgui glm cgltf stb microprofile)
target_compile_definitions(mclaren PRIVATE VK_NO_PROTOTYPES)
target_compile_definitions(mclaren PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_compile_definitions(mclaren PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_compile_options(mclaren PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fno-exceptions>
    $<$<CXX_COMPILER_ID:Clang>:-fno-exceptions>
    $<$<CXX_COMPILER_ID:MSVC>:/EHsc>
)

if (IOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_IOS)
elseif (APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_MACOS)
elseif (__ANDROID__)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_ANDROID)
elseif (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
endif ()

if (IOS)
elseif (APPLE)
    if (NOT DEFINED ENV{VULKAN_SDK})
        message(FATAL_ERROR "VULKAN_SDK environment variable is not set")
    endif ()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ENV{VULKAN_SDK}/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif ()
