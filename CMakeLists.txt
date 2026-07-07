cmake_minimum_required(VERSION 3.10)
set(PACKAGE_NAME Plogging_HW)

project(${PACKAGE_NAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

set(SOURCES
    src/main.cpp
    src/event.cpp
    src/network.cpp
    src/sensor/camera.cpp
    src/sensor/ultrasonic.cpp
    src/sensor/gps.cpp
    src/sensor/ledStrip.cpp
    src/sensor/servo.cpp
    src/sensor/spectroscopy.cpp
    lib/as7265x/SparkFun_AS7265X.cpp
)

add_executable(
    ${PACKAGE_NAME}_main
    ${SOURCES}
)

target_include_directories(${PACKAGE_NAME}_main PRIVATE 
    include
    lib/as7265x
)

target_link_libraries(
    ${PACKAGE_NAME}_main
    wiringPi
    Threads::Threads
    curl
    mosquitto
)
