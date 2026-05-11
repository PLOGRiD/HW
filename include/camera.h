// camera.h

#ifndef CAMERA_H
#define CAMERA_H

#include <string>

class Camera {
public:
    Camera();
    ~Camera();

    bool init();

    // 사진 촬영
    // 반환값: 저장된 파일 경로
    std::string capture();

private:
    bool initialized;
};

#endif