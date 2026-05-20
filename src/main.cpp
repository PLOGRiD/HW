#include "camera.h"
#include <iostream>

int main() {

    Camera cam;
    int input = 0;
    bool eventDetected = false;

    if (!cam.init()) {
        return -1;
    }

    std::cout<<"input 1\n";
    std::cin>>input;

    if (input == 1){
        eventDetected = true;
    }

    if (eventDetected) {

        std::string imagePath = cam.capture();

        if (!imagePath.empty()) {
            std::cout << "Image captured\n";
        }
    }

    return 0;
}