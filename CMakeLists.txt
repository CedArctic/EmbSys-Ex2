cmake_minimum_required(VERSION 3.16)
project(EmbSys_Ex2 C)

set(CMAKE_C_STANDARD 99)

# Configure cross compiler
#set(CMAKE_SYSTEM_NAME Linux)
#set(CMAKE_SYSTEM_PROCESSOR arm)
unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)

# Pthreads library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(EmbSys_Ex2 main.c src/queue/queue.h src/queue/queue.c src/timer/timer.h src/timer/timer.c src/csv/csv.h src/csv/csv.c)

# Link pthreads and math libraries
target_link_libraries(EmbSys_Ex2 PRIVATE Threads::Threads m)