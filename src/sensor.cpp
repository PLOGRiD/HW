#include "sensor.h"
#include "config.h"
#include <wiringPi.h>
#include <iostream>

Ultrasonic::Ultrasonic(int trig, int echo)
    : trig_pin(trig), echo_pin(echo) {}
    

void Ultrasonic::init_sensor() {
    if (wiringPiSetup() == -1) {
        std::cerr << "[wiringPi] Error: set gpio\n";
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
    unsigned int time = micros();

    while (digitalRead(echo_pin) == HIGH);
    time = micros() - time;

    double distance = time / 58.0;

    return distance;
}

void Ultrasonic::test_work(int delay_time){
    
    while(true){
        std::cout << "[test_ultrasonic] distance: " << get_distance() << "\n";
        delay(delay_time);
    }
}