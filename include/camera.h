#pragma once

#include <string>

class Camera {
public:
    Camera();
    ~Camera();

    bool init();

    std::string capture();

private:
    bool initialized;
};