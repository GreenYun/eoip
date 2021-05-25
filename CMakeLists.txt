cmake_minimum_required(VERSION 3.10)

project(eoip)

file(GLOB_RECURSE eoip_SOURCES "src/*.c")

add_executable(eoip ${eoip_SOURCES})
target_include_directories(eoip PRIVATE ${PROJECT_SOURCE_DIR}/include)

set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set_property(TARGET eoip PROPERTY C_STANDARD 99)
set_property(TARGET eoip PROPERTY C_STANDARD_REQUIRED ON)

install(TARGETS eoip DESTINATION bin)
