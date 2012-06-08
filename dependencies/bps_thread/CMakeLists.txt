if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "-Werror -fexceptions -fPIC ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-Werror -fPIC ${CMAKE_C_FLAGS}")
endif ()

set(bpsevent_srcs
    BPSMaster.cpp
)

include_directories(${CMAKE_SOURCE_DIR})

add_definitions(-DXP_UNIX)
add_library(bpsevent MODULE ${bpsevent_srcs})
target_link_libraries(bpsevent bps)

set_target_properties(bpsevent PROPERTIES PREFIX "")

