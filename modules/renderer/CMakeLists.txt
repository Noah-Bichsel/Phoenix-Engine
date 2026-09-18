# --- renderer ---------------------------------------------------------------
# Rendering interface. Consumers include <renderer/renderer.h> and link
# renderer; the backends under src/ are private to this library.
#
# Vulkan::Vulkan, glfw, glm::glm and assimp::assimp are provided by the root
# CMakeLists.txt, so this must be added after those are found/fetched.

add_library(renderer STATIC
        include/renderer/renderer.h
        src/vulkan/vk_renderer.cpp
        src/vulkan/vk_renderer.h
        src/vulkan/Utilites.h
        src/vulkan/validationLayers.h
        src/Mesh.cpp
        include/renderer/Mesh.h
        src/MeshModel.cpp
        include/renderer/MeshModel.h
        src/renderer.cpp)

add_library(Phoenix::renderer ALIAS renderer)

target_compile_features(renderer PUBLIC cxx_std_20)

target_include_directories(renderer
        PUBLIC
        include
        PRIVATE
        src/vulkan
        ${PROJECT_SOURCE_DIR}/third_party)   # stb_image.h

# Vulkan clip space is [0, 1] depth. Public so every translation unit that
# builds a glm projection agrees, not just the ones inside this library.
target_compile_definitions(renderer PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

# stb_image's implementation must be compiled into exactly one translation unit.
set_source_files_properties(src/vulkan/vk_renderer.cpp
        PROPERTIES COMPILE_DEFINITIONS STB_IMAGE_IMPLEMENTATION)

target_link_libraries(renderer
        PUBLIC
        glfw
        glm::glm
        PRIVATE
        Vulkan::Vulkan
        assimp::assimp)
