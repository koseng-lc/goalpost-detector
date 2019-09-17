project(goalpost_detector)
cmake_minimum_required(VERSION 2.8.3)
add_compile_options(-std=c++11)

find_package(OpenCV REQUIRED)

add_executable(test goalpost_detector.cpp)

target_link_libraries(test ${OpenCV_LIBRARIES})
