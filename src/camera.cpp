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

    // 💡 [수정 1] 카메라 포맷을 MJPG로 설정 (고해상도에서도 빠른 프레임 처리를 위해 필수)
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));

    // 💡 [수정 2] 해상도 설정 주석 해제 및 고해상도 할당
    // 시야가 잘리지 않는 4:3 비율의 고해상도를 입력해야 Full FOV를 얻을 수 있습니다.
    // config.h에 정의된 FRAME_WIDTH가 낮다면 아래처럼 강제로 높여보세요.
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);  // 또는 2592, 1280 등 카메라 스펙에 맞춰서
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1440); // 또는 1944, 960 등 

    cap.set(cv::CAP_PROP_AUTOFOCUS, 1);

    // 예열용 프레임 읽어오기
    cv::Mat dummy;
    for(int i = 0; i < 40; i++) {
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
    
    // LED가 켜진 후, 카메라가 빛에 적응하고(Auto-Exposure) 
    // 버퍼에 쌓인 어두운 과거 프레임을 모두 버리기 위해 충분히 프레임을 읽어냅니다.
    // 해상도가 높아졌기 때문에 10프레임 정도만 날려도 충분할 수 있습니다.
    for (int i = 0; i < 15; i++) {
        cap.read(frame); 
    }

    // 완전히 빛에 적응된 최신 프레임 획득
    if (!cap.read(frame) || frame.empty()) {
        std::cerr << "[Camera] Capture failed\n";
        return "";
    }

    std::time_t now = std::time(nullptr);
    std::string filename = "./images/" + std::to_string(now) + ".jpg";

    // 💡 [수정 3] jpeg 저장 화질을 최상(100)으로 설정
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);

    if (!cv::imwrite(filename, frame, compression_params)) {
        std::cerr << "[Camera] Failed to save image\n";
        return "";
    }

    std::cout << "[Camera] Saved: " << filename << "\n";

    return filename;
}