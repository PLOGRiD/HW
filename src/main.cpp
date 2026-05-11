#include <iostream>
#include <wiringPi.h>
#include "sensor.h"
#include "config.h"


int main(void){
    if (wiringPiSetup() == -1) {
        std::cerr << "[wiringPi] Error: set gpio\n";
        return;
    }

    return 0;
}
