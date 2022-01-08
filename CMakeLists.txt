cmake_minimum_required(VERSION 3.21)
project(StorageServer C)

set(CMAKE_C_STANDARD 99)

add_executable(StorageServer main.c headers/api.h datastructures/node.c datastructures/list.c headers/node.h headers/list.h)
