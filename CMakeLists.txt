cmake_minimum_required(VERSION 2.8)

project("XPointer Forwarding" C)

add_executable(forward forward.c)
add_executable(wtest wtest.c)
add_executable(sotest sotest.c)

target_link_libraries(forward -lxcb)
target_link_libraries(wtest -lxcb)
target_link_libraries(sotest -lxcb)

target_compile_options(forward PRIVATE -Wall -Wextra)
target_compile_options(wtest PRIVATE -Wall -Wextra)
target_compile_options(sotest PRIVATE -Wall -Wextra)
