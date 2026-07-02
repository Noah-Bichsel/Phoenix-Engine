cmake_minimum_required(VERSION 4.3)
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
        VulkanValidation.h)

target_link_libraries(VulkanCourseApp____Clion
        Vulkan::Vulkan
        glfw)

add_custom_command(TARGET VulkanCourseApp____Clion POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:VulkanCourseApp____Clion>/Shaders)
