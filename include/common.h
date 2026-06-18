#pragma once

#include <string>
#include <queue>
#include <mutex>
#include "sensor.h"

struct PloggingData {
    std::string image_path;
    SpectroscopyData spec_data;
};

// 공유 자원
extern std::queue<PloggingData> uploadQueue; // http 전송 큐
extern std::mutex queueMutex; // 큐 뮤텍스
