cmake_minimum_required(VERSION 3.10)

project(Mole)

include_directories(include)
if(WIN32)
    add_compile_options(/utf-8)
endif()
add_library(libmole SHARED src/Mole.cpp)
add_executable(Mole src/Mole.cpp main.cpp)
target_link_libraries(libmole PUBLIC pthread)
target_link_libraries(Mole PRIVATE pthread)
# fmt
find_package(fmt CONFIG QUIET)
if(fmt_FOUND)
    message(STATUS "find package fmt")
    target_link_libraries(libmole PUBLIC fmt::fmt)
    target_link_libraries(Mole PRIVATE fmt::fmt)
else()
    include(FetchContent)
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 10.2.1
    )
    FetchContent_MakeAvailable(fmt)
    target_link_libraries(libmole PUBLIC fmt)
    target_link_libraries(Mole PRIVATE fmt)
endif()


include(FetchContent)
FetchContent_Declare(
        concurrentqueue
        GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
        GIT_TAG v1.0.4
)
FetchContent_MakeAvailable(concurrentqueue)
set_target_properties(libmole PROPERTIES PREFIX "")
target_link_libraries(libmole PUBLIC concurrentqueue)
target_link_libraries(Mole PRIVATE concurrentqueue)

target_include_directories(libmole PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(libmole PROPERTIES EXPORT_NAME Mole)

install(TARGETS libmole EXPORT MoleTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT MoleTargets FILE MoleConfig.cmake DESTINATION lib/cmake/Mole)