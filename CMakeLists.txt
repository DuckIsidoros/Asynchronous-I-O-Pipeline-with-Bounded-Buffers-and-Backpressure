cmake_minimum_required(VERSION 3.16)

project(AsynchronousIOPipeline LANGUAGES C)

# Require C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Find pthread support
find_package(Threads REQUIRED)

# Collect C source files
file(GLOB SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Build executable
add_executable(pipeline ${SOURCES})

# Include project headers
target_include_directories(pipeline PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Enable strict compiler warnings
target_compile_options(pipeline PRIVATE
    -Wall
    -Wextra
    -Werror
    -pedantic
)

# Link pthread support
target_link_libraries(pipeline PRIVATE
    Threads::Threads
)
