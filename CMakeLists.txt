cmake_minimum_required(VERSION 4.0)
project(sandbox VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)

include_directories(include/)

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(log4cxx CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyfiledialogs CONFIG REQUIRED)

add_compile_options("-Wall")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>" "$<$<CXX_COMPILER_ID:MSVC>:/execution-charset:utf-8>")

add_executable(sandbox src/main.cpp)

if (WIN32)
    set_target_properties(sandbox PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif ()

if (MSVC)
    target_sources(sandbox PRIVATE FILE_SET msvcStdModule TYPE CXX_MODULES BASE_DIRS $ENV{VCToolsInstallDir}
            FILES $ENV{VCToolsInstallDir}/modules/std.ixx)
endif ()

target_sources(sandbox PRIVATE FILE_SET CXX_MODULES BASE_DIRS .
        FILES
        src/camera.ixx
        src/dialog.ixx
        src/io.ixx
        src/log.ixx
        src/opengl.ixx
        src/timer.ixx
        src/window.ixx
        src/gl/buffer.ixx
        src/gl/descriptor.ixx
        src/gl/pipeline.ixx
)

target_link_libraries(sandbox PRIVATE
        glad::glad
        glfw
        glm::glm
        log4cxx
        tinyfiledialogs::tinyfiledialogs)
target_include_directories(sandbox PRIVATE ${Stb_INCLUDE_DIR})

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} sandbox)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_INSTALL_BINDIR})
include(InstallRequiredSystemLibraries)

set(CPACK_GENERATOR ZIP)
include(CPack)
