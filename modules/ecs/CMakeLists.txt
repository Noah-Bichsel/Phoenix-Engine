# --- ecs --------------------------------------------------------------------
# Entity-Component-System (Scene, Object, Components, Behaviour). Consumers
# include <ecs/...> and link ecs.
#
# EnTT::EnTT is provided by the root CMakeLists.txt, and core / renderer by
# modules/core and modules/renderer, so this must be added after those.

add_library(ecs STATIC
        include/ecs/Behaviour.h
        include/ecs/Component.h
        include/ecs/Object.h
        include/ecs/Scene.h
        include/ecs/src/Scene.cpp
        include/ecs/SceneManager.h)

add_library(Phoenix::ecs ALIAS ecs)

target_compile_features(ecs PUBLIC cxx_std_20)

target_include_directories(ecs
        PUBLIC
        include
        PRIVATE
        # So this module's own sources can include its headers unqualified,
        # e.g. "Scene.h" instead of <ecs/Scene.h>. Private, so it never leaks
        # that short spelling to consumers.
        include/ecs)

# Component.h, Object.h and Scene.h expose EnTT, Vector3 (core) and Renderer
# (renderer) in public headers, so all three are PUBLIC.
target_link_libraries(ecs
        PUBLIC
        core
        renderer
        EnTT::EnTT)
