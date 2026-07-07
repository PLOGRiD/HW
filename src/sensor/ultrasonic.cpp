#include "sensor/ultrasonic.h"
#include "config.h"
#include <wiringPi.h>
#include <iostream>

using namespace std;

Ultrasonic::Ultrasonic(int trig, int echo)
    : trig_pin(trig), echo_pin(echo), base_distance(0), count(0) {}
    

void Ultrasonic::init() {
    pinMode(trig_pin, OUTPUT);
    pinMode(echo_pin, INPUT);

    digitalWrite(trig_pin, LOW);
    std::cout<<"[ultrasonic] Initialized\n";
}

double Ultrasonic::get_distance() {
    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_pin, LOW);

    unsigned int start = micros();
    while (digitalRead(echo_pin) == LOW) {
        if (micros() - start > 30000) return -1;
    }

    start = micros();
    while (digitalRead(echo_pin) == HIGH) {
        if (micros() - start > 30000) return -1;
    }
    unsigned int time = micros() - start;

    double distance = time / 58.0;

    return distance;
}

void Ultrasonic::set_base(){
    double sum = 0;
    double distance;

    for(int i = 0; i < 10; i++){
        distance = get_distance();
        if (distance < 0){
            i--;
            continue;
        }
        sum += distance;
        delay(50);
    }

    base_distance = sum / 10.0;
}

bool Ultrasonic::detect_event(double distance){
    if (distance < 0){
        cerr<<"get_distance Error\n";
        return false;
    }

    bool isChanged = (distance < base_distance - DETECT_GAP);

    if (isChanged) {
        count++;
        // if (count >= DETECT_COUNT) {
        //     count = 0;
        //     return true;
        // }
        return true;
    }
    // else {
    //     count = 0;
    // }

    return false;
}

void Ultrasonic::test_work(int delay_time){
    
    while(true){
        cout << "[test_ultrasonic] distance: " << get_distance() << "\n";
        delay(delay_time);
    }
};