#include "camera.h"
#include <iostream>
#include "sensor.h"
#include "config.h"

int main() {
    if (wiringPiSetup() == -1) {
        std::cerr << "[wiringPi] Error: set gpio\n";
        return;
    }
    // SpectroscopySensor sensor;

    // if (!sensor.init()){
    //     return -1;
    // }

    // sensor.collectDataForAI();

    Camera cam;

    if (!cam.init()) {
        return -1;
    }

    // 이벤트 발생했다고 가정
    int eventDetected;

    while(true){
        std::cin>>eventDetected;

        if (eventDetected) {
            std::string imagePath = cam.capture();

            if (!imagePath.empty()) {
                std::cout << "Image captured\n";
            }
        }
    }
    return 0;
}
