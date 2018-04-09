cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_COMPILER "gcc-7")
set(CMAKE_CXX_COMPILER "g++-7")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(simperium-c)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(SOURCES
    src/simperium_auth.c
    src/simperium_bucket.c
    src/simperium_bucket_http.c
    src/simperium_bucket_ws.c
    )

add_library(simperium ${SOURCES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(simperium ${CONAN_LIBS})

add_executable(todo_add_remove "${PROJECT_SOURCE_DIR}/examples/todo_add_remove/todo_add_remove.c")
target_link_libraries(todo_add_remove simperium)
target_link_libraries(todo_add_remove ${CONAN_LIBS})

add_executable(todo_ws "${PROJECT_SOURCE_DIR}/examples/todo_ws/todo_ws.c")
target_link_libraries(todo_ws simperium)
target_link_libraries(todo_ws ${CONAN_LIBS})

add_executable(todo_changes "${PROJECT_SOURCE_DIR}/examples/todo_changes/todo_changes.c")
target_link_libraries(todo_changes simperium)
target_link_libraries(todo_changes ${CONAN_LIBS})

#enable_testing()
#add_executable(simperium-tests test/test_main.cpp test/test_apply.cpp test/test_diff.cpp)
#target_link_libraries(simperium-tests ${CONAN_LIBS})
#target_link_libraries(simperium-tests simperium)
#add_test(NAME run-tests
#         COMMAND simperium-tests)