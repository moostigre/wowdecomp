file(GLOB_RECURSE PRIVATE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(FrameScript STATIC
    ${PRIVATE_SOURCES}
)

target_include_directories(FrameScript
    PUBLIC
        ${CMAKE_SOURCE_DIR}
)

# Lifecycle notifications use the portable module host.
target_link_libraries(FrameScript PUBLIC ClientModules)
