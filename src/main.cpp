#include <iostream>
#include "sensor.h"
#include "config.h"


int main(void){
    Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);

    return 0;
}
