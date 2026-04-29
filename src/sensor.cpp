#include "sensor.h"
#include <wiringPi.h>
#include <iostream>

Ultrasonic::Ultrasonic(int trig, int echo)
    : trig_pin(trig), echo_pin(echo) {}

void Ultrasonic::init_sensor() {
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "[wiringPi] Error: set gpio" << std::endl;
        return;
    }

    pinMode(trig_pin, OUTPUT);
    pinMode(echo_pin, INPUT);

    digitalWrite(trig_pin, LOW);
}

double Ultrasonic::get_distance() {
    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_pin, LOW);

    while (digitalRead(echo_pin) == LOW);
    unsigned int start = micros();

    while (digitalRead(echo_pin) == HIGH);
    unsigned int end = micros();

    unsigned int time = end - start;

    return time / 58.0;
}

void Ultrasonic::test_work(){
    while(true){
        std::cout << "[test_ultrasonic] distance: " << get_distance() << "\n";
        delay(100);
    }
}