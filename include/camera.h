#pragma once

#include <string>
#include <opencv2/opencv.hpp>

class Camera {
public:
    Camera();
    ~Camera();

    bool init();

    std::string capture();

private:
    bool initialized;
    cv::VideoCapture cap;
};