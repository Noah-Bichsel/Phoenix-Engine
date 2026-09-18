# --- editor -----------------------------------------------------------------
# The editor executable: an Application subclass plus the entry point. Links the
# engine modules; nothing links back to it.
#
# The target keeps the name PhoenixEngine so the asset-copy target in the root
# CMakeLists and the CLion run configuration keep working.

add_executable(PhoenixEngine
        main.cpp
        src/EditorApp.h
        src/EditorApp.cpp)

target_link_libraries(PhoenixEngine
        PRIVATE
        core
        application
        renderer
        engine
        Vulkan::Vulkan)
