cmake_minimum_required(VERSION 3.10)
set(PACKAGE_NAME Plogging_HW)

project(${PACKAGE_NAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

set(SOURCES
    src/main.cpp
    src/sensor.cpp
    src/camera.cpp
    src/event.cpp
    src/network.cpp
    lib/as7265x/SparkFun_AS7265X.cpp
)

add_executable(
    ${PACKAGE_NAME}_main
    ${SOURCES}
)

target_include_directories(${PACKAGE_NAME}_main PRIVATE 
    include
    lib/as7265x
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(
    ${PACKAGE_NAME}_main
    wiringPi
    ${OpenCV_LIBS}
    Threads::Threads
    curl
)
