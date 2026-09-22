#pragma once

#include "sensor/spectroscopy.h"
#include <string>
#include <queue>
#include <mutex>

struct GpsData {
    double latitude = 0.0;
    double longitude = 0.0;
    int64_t timestamp = 0;
    bool isValid = false;
};

struct PloggingData {
    std::string image_path;
    SpectroscopyData spec_data;
    GpsData gps_location;
    int64_t gps_age_sec;
    std::string timestamp;
};

// 공유 자원
extern std::queue<PloggingData> uploadQueue; // http 전송 큐
extern std::mutex queueMutex; // 큐 뮤텍스

extern GpsData globalGpsData; // gps 공유 변수
extern std::mutex gpsMutex; // gps 뮤텍스