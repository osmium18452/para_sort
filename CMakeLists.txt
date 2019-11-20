cmake_minimum_required(VERSION 3.15)
project(sort)

set(CMAKE_CXX_STANDARD 14)

find_package(MPI REQUIRED)

SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpicxx)

INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
INCLUDE_DIRECTORIES("C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Include")

ADD_EXECUTABLE(sort main.cpp)