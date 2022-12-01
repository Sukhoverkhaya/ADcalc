cmake_minimum_required(VERSION 3.0.0)
project(pulsedetcpp VERSION 0.1.0)

include(CTest)
enable_testing()

# include_directories(from webdevice)

add_executable(pulsedetcpp ./Alg/main.cpp)
# add_executable(pulsedetcpp "./drafts/draft.cpp")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
