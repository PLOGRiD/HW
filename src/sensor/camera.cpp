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
    std::string init_cmd = "rpicam-still -t 0 -s -n --lens-position 4.0 --width 1920 --height 1080 -q 90 -o ./images/latest.jpg &";
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

    std::string temp_file = "./images/latest.jpg";
    if (std::filesystem::exists(temp_file)) {
        std::filesystem::remove(temp_file);
    }

    // 시그널 전송으로 촬영 지시
    system("killall -SIGUSR1 rpicam-still");

    int timeout = 0;
    bool capture_success = false;
    std::error_code ec;

    // ✨핵심 수정: 파일이 존재할 뿐만 아니라, 용량이 10KB 이상 채워질 때까지 대기
    while (timeout < 40) { // 최대 2초 대기 (50ms * 40)
        if (std::filesystem::exists(temp_file)) {
            // 파일 쓰기가 완료되어 용량이 확보되었는지 확인
            if (std::filesystem::file_size(temp_file, ec) > 10000) { 
                capture_success = true;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        timeout++;
    }

    if (!capture_success) {
        std::cerr << "[Camera] Capture timeout! Signal failed or file too small.\n";
        return "";
    }

    // 안전빵으로 쓰기 버퍼가 완전히 닫힐 때까지 100ms 추가 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::time_t now = std::time(nullptr);
    std::string final_filename = "./images/" + std::to_string(now) + ".jpg";
    
    std::filesystem::rename(temp_file, final_filename);

    std::cout << "[Camera] Saved: " << final_filename << "\n";

    return final_filename;
}