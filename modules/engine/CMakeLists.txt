# --- engine -----------------------------------------------------------------
# Engine-level systems built on top of application + renderer: the layers an app
# pushes onto its LayerStack. Consumers include <engine/...> and link engine.
#
# Sits above application, renderer and core, so this must be added after them.

add_library(engine STATIC
        include/engine/CoreLayer.h
        src/CoreLayer.cpp
        include/engine/RenderingLayer.h
        src/RenderingLayer.cpp)

add_library(Phoenix::engine ALIAS engine)

target_compile_features(engine PUBLIC cxx_std_20)

target_include_directories(engine
        PUBLIC
        include)

# RenderingLayer.h includes <application/ILayer.h> and <renderer/renderer.h>, so
# anything that includes it needs both as well - hence PUBLIC. core and glfw are
# only used from the .cpp files, so they stay PRIVATE.
target_link_libraries(engine
        PUBLIC
        application
        renderer
        PRIVATE
        core
        glfw)
