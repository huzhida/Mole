cmake_minimum_required(VERSION 3.10)
project(Mole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(include)
add_executable(mole.test main.cpp src/Mole.cpp)
add_library(mole SHARED src/Mole.cpp)
add_library(mole.static STATIC src/Mole.cpp)


# fmt
find_package(fmt CONFIG QUIET)
if(fmt_FOUND)
    message(STATUS "find package fmt")
    target_link_libraries(mole PUBLIC fmt::fmt)
    target_link_libraries(mole.static PUBLIC fmt)
    target_link_libraries(mole.test PUBLIC fmt::fmt)
else()
    include(FetchContent)
    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 10.2.1
    )
    FetchContent_MakeAvailable(fmt)
    target_link_libraries(mole PUBLIC fmt)
    target_link_libraries(mole.static PUBLIC fmt)
    target_link_libraries(mole.test PUBLIC fmt)
endif()


include(FetchContent)
FetchContent_Declare(
        concurrentqueue
        GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
        GIT_TAG v1.0.4
)
FetchContent_MakeAvailable(concurrentqueue)
target_link_libraries(mole PUBLIC concurrentqueue)
target_link_libraries(mole.test PUBLIC concurrentqueue)
target_link_libraries(mole.static PUBLIC concurrentqueue)

if(UNIX)
    target_link_libraries(mole PUBLIC pthread)
    target_link_libraries(mole.test PUBLIC pthread)
    target_link_libraries(mole.static PUBLIC pthread)
endif()

target_include_directories(mole
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(mole.static
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_library(Mole::Mole ALIAS mole)

include(CMakePackageConfigHelpers)

install(
        TARGETS mole
        EXPORT MoleTargets
        INCLUDES DESTINATION include
)

install(
        DIRECTORY include/
        DESTINATION include
)

install(
        EXPORT MoleTargets
        FILE MoleTargets.cmake
        NAMESPACE Mole::
        DESTINATION lib/cmake/Mole
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/MoleConfigVersion.cmake
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)

install(
        FILES
        ${CMAKE_CURRENT_LIST_DIR}/cmake/MoleConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/MoleConfigVersion.cmake
        DESTINATION lib/cmake/Mole
)
