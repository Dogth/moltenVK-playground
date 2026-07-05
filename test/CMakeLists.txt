find_package(Catch2 3 REQUIRED)
include(CTest)
include(Catch)
 
add_executable(unit_tests test_init.cpp)
target_link_libraries(unit_tests PRIVATE vulkan_app Catch2::Catch2WithMain)
 
catch_discover_tests(unit_tests)
 
find_program(SPIRV_VAL spirv-val REQUIRED)
 
foreach(SPV ${SHADER_SPV_FILES})
    get_filename_component(SPV_NAME ${SPV} NAME)
    add_test(NAME "spirv_val.${SPV_NAME}" COMMAND ${SPIRV_VAL} ${SPV})
endforeach()
 
