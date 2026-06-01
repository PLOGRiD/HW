#include "camera.h"
#include "config.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <vector>

Camera::Camera() {
    initialized = false;
}

Camera::~Camera() {
    if (cap.isOpened()) {
        cap.release();
    }
}

bool Camera::init() {

    // 저장 폴더 없으면 생성
    std::filesystem::create_directory("./images");

    cap.open(0, cv::CAP_V4L2);
    
    if (!cap.isOpened()) {
        std::cerr << "[Camera] Failed to open camera directly\n";
        return false;
    }

    // cap.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    // cap.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

    // 예열용 프레임 읽어오기
    cv::Mat dummy;
    for(int i = 0; i < 10; i++) {
        cap.read(dummy);
    }

    initialized = true;

    std::cout << "[Camera] Initialized\n";

    return true;
}

std::string Camera::capture() {

    if (!initialized) {
        std::cerr << "[Camera] Not initialized\n";
        return "";
    }

    cv::Mat frame;
    
    // 이전 프레임 삭제
    cap.grab(); 
    cap.grab();
    
    // api 호출
    if (!cap.read(frame) || frame.empty()) {
        std::cerr << "[Camera] Capture failed\n";
        return "";
    }

    std::time_t now = std::time(nullptr);
    std::string filename = "./images/" + std::to_string(now) + ".jpg";

    // jpeg로 압축 및 저장
    if (!cv::imwrite(filename, frame)) {
        std::cerr << "[Camera] Failed to save image\n";
        return "";
    }

    std::cout << "[Camera] Saved: " << filename << "\n";

    return filename;
}