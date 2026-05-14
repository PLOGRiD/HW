#include "camera.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <filesystem>

Camera::Camera() {
    initialized = false;
}

Camera::~Camera() {
}

bool Camera::init() {

    // 저장 폴더 없으면 생성
    std::filesystem::create_directory("./images");

    initialized = true;

    std::cout << "[Camera] Initialized\n";

    return true;
}

std::string Camera::capture() {

    if (!initialized) {
        std::cerr << "[Camera] Not initialized\n";
        return "";
    }

    // 현재 시간 기반 파일명 생성
    std::time_t now = std::time(nullptr);

    std::string filename =
        "./images/" + std::to_string(now) + ".jpg";

    std::string command =
        "libcamera-jpeg "
        "-n "
        "--width 1920 "
        "--height 1080 "
        "-o " + filename;

    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "[Camera] Capture failed\n";
        return "";
    }

    std::cout << "[Camera] Saved: "
              << filename << "\n";

    return filename;
}