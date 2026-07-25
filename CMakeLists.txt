cmake_minimum_required(VERSION 3.20)

# --- Local dependency bootstrap (Windows dev machine) ---------------------
# These make the project configure without needing any CLion CMake settings.
# Each block is guarded by EXISTS, so it's a harmless no-op on other machines.

# Use the vcpkg toolchain (provides glfw3) if one wasn't supplied.
# Must be set before the first project() call.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
endif()

# Point FindVulkan at the SDK even if VULKAN_SDK isn't in the environment yet
# (e.g. CLion inherited a stale env before the SDK was installed). Picks the
# newest SDK under C:/VulkanSDK.
if(NOT DEFINED ENV{VULKAN_SDK} AND EXISTS "C:/VulkanSDK")
    file(GLOB _vk_sdks LIST_DIRECTORIES true "C:/VulkanSDK/*")
    if(_vk_sdks)
        list(SORT _vk_sdks)
        list(GET _vk_sdks -1 _vk_sdk)
        set(ENV{VULKAN_SDK} "${_vk_sdk}")
    endif()
endif()
# -------------------------------------------------------------------------

project(VulkanCourseApp____Clion)

set(CMAKE_CXX_STANDARD 14)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(.)

add_executable(VulkanCourseApp____Clion
        main.cpp
        Utilites.h
        VulkanRenderer.cpp
        VulkanRenderer.h
        VulkanValidation.h
        Mesh.cpp
        Mesh.h)

target_link_libraries(VulkanCourseApp____Clion
        Vulkan::Vulkan
        glfw)

add_custom_command(TARGET VulkanCourseApp____Clion POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:VulkanCourseApp____Clion>/Shaders)
