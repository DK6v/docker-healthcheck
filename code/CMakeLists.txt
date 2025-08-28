cmake_minimum_required(VERSION 3.10)
project(portcheck)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LINKER_FLAGS -static-libgcc -static-libstdc++ -static)

add_executable(TCP main.cpp)
target_link_libraries(TCP PRIVATE ${LINKER_FLAGS})
set_target_properties(TCP PROPERTIES OUTPUT_NAME "TCP")
target_compile_definitions(TCP PRIVATE NET_STAT_FILE_PATH="/proc/net/tcp")

add_executable(UDP main.cpp)
target_link_libraries(UDP PRIVATE ${LINKER_FLAGS})
set_target_properties(UDP PROPERTIES OUTPUT_NAME "UDP")
target_compile_definitions(UDP PRIVATE NET_STAT_FILE_PATH="/proc/net/udp")

add_executable(TCP6 main.cpp)
target_link_libraries(TCP6 PRIVATE ${LINKER_FLAGS})
set_target_properties(TCP6 PROPERTIES OUTPUT_NAME "TCP6")
target_compile_definitions(TCP6 PRIVATE NET_STAT_FILE_PATH="/proc/net/tcp6" USE_IPV6)

add_executable(UDP6 main.cpp)
target_link_libraries(UDP6 PRIVATE ${LINKER_FLAGS})
set_target_properties(UDP6 PROPERTIES OUTPUT_NAME "UDP6")
target_compile_definitions(UDP6 PRIVATE NET_STAT_FILE_PATH="/proc/net/udp6" USE_IPV6)
