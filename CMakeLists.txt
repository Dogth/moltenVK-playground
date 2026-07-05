cmake_minimum_required(VERSION 3.20)
project(vulkan_moltenvk_demo LANGUAGES CXX)
 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
 
option(BUILD_TESTING "Build tests" ON)

find_package(Vulkan REQUIRED)
find_package(Glfw3 REQUIRED)
 
add_library(vulkan_app
    src/init.cpp
		src/pipeline.cpp
		src/vk_types.cpp
)

target_include_directories(vulkan_app PUBLIC include)
target_link_libraries(vulkan_app PUBLIC Vulkan::Vulkan Glfw)
 
if(APPLE)
    target_compile_definitions(vulkan_app PUBLIC VK_APP_USE_PORTABILITY)
endif()
 
add_executable(vulkan_demo src/main.cpp)
target_link_libraries(vulkan_demo PRIVATE vulkan_app)
 
find_program(GLSLC glslc REQUIRED)
 
set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shader)
set(SHADER_BIN_DIR ${CMAKE_SOURCE_DIR}/shader/bin)
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})
 
set(SHADER_SOURCES
    ${SHADER_SRC_DIR}/schlick-fresnel/sf.vert
    ${SHADER_SRC_DIR}/schlick-fresnel/sf.frag
)
 
set(SHADER_SPV_FILES "")
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPV_OUT ${SHADER_BIN_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPV_OUT}
        COMMAND ${GLSLC} ${SHADER} -o ${SPV_OUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER_NAME}"
        VERBATIM
    )
    list(APPEND SHADER_SPV_FILES ${SPV_OUT})
endforeach()
 
add_custom_target(shaders ALL DEPENDS ${SHADER_SPV_FILES})
add_dependencies(vulkan_demo shaders)
 
target_compile_definitions(vulkan_app PRIVATE SHADER_DIR="${SHADER_BIN_DIR}")

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
