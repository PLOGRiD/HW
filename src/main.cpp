#include "camera.h"
#include <iostream>
#include "sensor.h"

int main() {
    SpectroscopySensor sensor;

    if (!sensor.init()){
        return -1;
    }

    sensor.collectDataForAI();

    // Camera cam;

    // if (!cam.init()) {
    //     return -1;
    // }

    // // 이벤트 발생했다고 가정
    // bool eventDetected = true;

    // if (eventDetected) {

    //     std::string imagePath = cam.capture();

    //     if (!imagePath.empty()) {
    //         std::cout << "Image captured\n";
    //     }
    // }

    return 0;
}