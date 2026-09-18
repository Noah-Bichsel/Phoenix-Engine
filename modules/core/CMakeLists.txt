# --- core -------------------------------------------------------------------
# Low-level engine utilities (time, ...). Consumers include <core/...> and link
# core.
#
# glfw is provided by the root CMakeLists.txt, so this must be added after it
# is found.
#
# Header-only for now, so this is an INTERFACE library (listing the headers
# keeps them visible in the IDE). Once a .cpp is added, e.g. src/time.cpp,
# change INTERFACE to STATIC on add_library and to PUBLIC on the three target_*
# calls below, and add the .cpp to the source list.

add_library(core INTERFACE
        include/core/Time.h)

add_library(Phoenix::core ALIAS core)

target_compile_features(core INTERFACE cxx_std_20)

target_include_directories(core
        INTERFACE
        include)

# Nothing to link: time.h is pure <chrono> now, so core depends on nothing.
