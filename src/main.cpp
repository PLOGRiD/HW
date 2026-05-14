#include "camera.h"
#include <iostream>

int main() {

    Camera cam;

    if (!cam.init()) {
        return -1;
    }

    // 이벤트 발생했다고 가정
    bool eventDetected = true;

    if (eventDetected) {

        std::string imagePath = cam.capture();

        if (!imagePath.empty()) {
            std::cout << "Image captured\n";
        }
    }

    return 0;
}