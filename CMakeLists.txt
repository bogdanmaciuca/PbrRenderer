cmake_minimum_required(VERSION 3.16)
project(PbrRenderer LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

file(GLOB SRCS
    src/*.cpp
    src/renderer/*.cpp
    src/imgui/*.cpp
)

include_directories(
    inc
    src
)

link_directories(lib)

add_executable(${PROJECT_NAME} ${SRCS} ${C_SRCS})

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    SDL3
    assimp-vc143-mt
)

# Precompiled headers
target_precompile_headers(
    ${PROJECT_NAME}
    PUBLIC src/pch.h
)

# Shader compilation (glslangValidator -> SPIRV)
set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_SOURCE_DIR}/shaders_compiled")

file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

# Find all shader files
file(GLOB SHADER_FILES
    "${SHADER_SRC_DIR}/*.vert"
    "${SHADER_SRC_DIR}/*.frag"
)

set(COMPILED_SHADERS)

foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV_FILE "${SHADER_OUT_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND glslangValidator -e main -V ${SHADER} -o ${SPIRV_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
        VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${SPIRV_FILE})
endforeach()

add_custom_target(compile_shaders ALL
    DEPENDS ${COMPILED_SHADERS}
)

