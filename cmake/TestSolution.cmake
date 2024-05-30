function(add_max_flow_executable NAME)
    add_executable(${NAME} ${ARGN})
    target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

function(add_catch TARGET)
    add_max_flow_executable(${TARGET} ${ARGN})
    target_link_libraries(${TARGET} contrib_catch_main)
endfunction()
