# --- application ------------------------------------------------------------
# Window / application setup. Consumers include <application/Application.h>
# and link application.
#
# glfw is provided by the root CMakeLists.txt, and renderer / core by
# modules/renderer and modules/core, so this must be added after those.

add_library(application STATIC
        include/application/Application.h
        src/Application.cpp
        include/application/Window.h
        src/Window.cpp
        include/application/ILayer.h
        include/application/LayerStack.h
        src/LayerStack.cpp)

add_library(Phoenix::application ALIAS application)

target_compile_features(application PUBLIC cxx_std_20)

target_include_directories(application
        PUBLIC
        include
        PRIVATE
        # So this module's own sources can include its headers unqualified,
        # e.g. "ILayer.h" instead of <application/ILayer.h>. Private, so it
        # never leaks that short spelling to consumers.
        include/application)

# Window.h includes <renderer/Renderer.h>, so consumers need renderer too - hence
# PUBLIC. GLFW is an implementation detail now: only Window.cpp includes it, and
# the public headers just forward declare GLFWwindow, so it stays PRIVATE. Vulkan
# isn't referenced by this module at all any more.
target_link_libraries(application
        PUBLIC
        renderer
        PRIVATE
        glfw
        core)
