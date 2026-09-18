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

project(PhoenixEngine)

# C++20 for our own code. assimp 6 needs at least C++17, and builds fine at 20,
# so it inherits this too rather than being pinned separately.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
# Header-only, but declare it: older LunarG SDKs bundled glm under their include
# dir and 1.4.x no longer does, so relying on the SDK to supply it silently
# breaks on an SDK upgrade.
find_package(glm REQUIRED)

# --- assimp ---------------------------------------------------------------
# Built from source as part of this project, so Windows and Linux behave
# identically and no prebuilt binaries / import libs have to be managed by
# hand. Prefer the vendored copy in external/assimp (gitignored); if it isn't
# there, FetchContent downloads the same v6.0.5 release archive.
include(FetchContent)

if(EXISTS "${CMAKE_SOURCE_DIR}/external/assimp/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_ASSIMP "${CMAKE_SOURCE_DIR}/external/assimp"
            CACHE PATH "Local assimp source tree")
endif()

# Static link: nothing to copy next to the executable at runtime.
set(BUILD_SHARED_LIBS OFF)

# Use assimp's bundled zlib/minizip rather than the system ones. Same on every
# platform, and it avoids distros (Arch) that install minizip's unzip.h under
# a subdirectory assimp doesn't add to its include path.
set(ASSIMP_BUILD_ZLIB ON)
set(ASSIMP_BUILD_MINIZIP ON)

# ASSIMP_BUILD_MINIZIP skips assimp's pkg_check_modules(UNZIP minizip), but a
# build tree configured before that flag was set still has UNZIP_FOUND=1 cached
# as INTERNAL. assimp reads it and then omits contrib/unzip, so the build dies
# on a missing <unzip.h>. Drop the stale entries so existing build dirs recover
# without having to be deleted.
foreach(_stale_unzip
        UNZIP_FOUND UNZIP_INCLUDE_DIRS UNZIP_INCLUDEDIR UNZIP_LIBRARIES
        UNZIP_LINK_LIBRARIES UNZIP_LIBRARY_DIRS UNZIP_LIBDIR UNZIP_LDFLAGS
        UNZIP_CFLAGS __pkg_config_checked_UNZIP)
    unset(${_stale_unzip} CACHE)
    unset(${_stale_unzip})
endforeach()

# Only the formats this project actually loads, otherwise assimp takes several
# minutes to compile. Add more here as needed (names match ADD_ASSIMP_IMPORTER
# calls in assimp's code/CMakeLists.txt, e.g. ASSIMP_BUILD_PLY_IMPORTER).
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_FBX_IMPORTER ON)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON)

set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_DOCS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)

FetchContent_Declare(assimp
        URL https://github.com/assimp/assimp/archive/refs/tags/v6.0.5.zip
        URL_HASH SHA256=89e477e89df739ecc7dd21874897ac2e70c8944eb1074fbdbbdbf4dd2de6a403
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(assimp)
# -------------------------------------------------------------------------

add_subdirectory(modules/core)
add_subdirectory(modules/renderer)
add_subdirectory(modules/application)
add_subdirectory(modules/engine)

# Defines the PhoenixEngine executable.
add_subdirectory(editor)

# Assets are opened by relative path at runtime ("Shaders/...", "Textures/...", "Models/..."),
# so they have to sit next to the executable. This is a custom target rather than a POST_BUILD
# step so it runs on every build - POST_BUILD only fires when the exe relinks, so editing just a
# shader would leave a stale .spv in the build dir.
add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:PhoenixEngine>/Shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Textures $<TARGET_FILE_DIR:PhoenixEngine>/Textures
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Models $<TARGET_FILE_DIR:PhoenixEngine>/Models)

add_dependencies(PhoenixEngine copy_assets)
