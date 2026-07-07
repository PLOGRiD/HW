#include "sensor/camera.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <chrono>

Camera::Camera() {
    initialized = false;
}

Camera::~Camera() {
    system("killall rpicam-still 2>/dev/null");
}

bool Camera::init() {
    std::filesystem::create_directory("./images");
    system("killall rpicam-still 2>/dev/null");

    std::cout << "[Camera] Starting background camera process (V3 AutoFocus)...\n";

    // 카메라 프로세스 준비 + 오토포커스
    std::string init_cmd = "rpicam-still -t 0 -s -n --lens-position 5.0 --width 1920 --height 1080 -q 100 -o ./images/latest.jpg &";
    system(init_cmd.c_str());

    std::this_thread::sleep_for(std::chrono::seconds(2));

    initialized = true;
    std::cout << "[Camera] Initialized\n";

    return true;
}

std::string Camera::capture() {
    if (!initialized) {
        std::cerr << "[Camera] Not initialized\n";
        return "";
    }

    // 이전 촬영본 삭제
    std::string temp_file = "./images/latest.jpg";
    if (std::filesystem::exists(temp_file)) {
        std::filesystem::remove(temp_file);
    }

    system("killall -SIGUSR1 rpicam-still");

    int timeout = 0;
    while (!std::filesystem::exists(temp_file) && timeout < 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        timeout++;
    }

    // 파일 쓰기 버퍼 처리 시간 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (timeout >= 20) {
        std::cerr << "[Camera] Capture timeout! Signal failed.\n";
        return "";
    }

    std::time_t now = std::time(nullptr);
    std::string final_filename = "./images/" + std::to_string(now) + ".jpg";
    
    std::filesystem::rename(temp_file, final_filename);

    std::cout << "[Camera] Saved: " << final_filename << "\n";

    return final_filename;
}