cmake_minimum_required(VERSION 3.1...3.15)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(libraryPrjA VERSION 1.0
                  DESCRIPTION "Test Library A"
                  LANGUAGES CXX)

add_library(liba SHARED libA.cpp libA.h)
