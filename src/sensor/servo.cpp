#include "sensor/servo.h"
#include "config.h"
#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <thread>
#include <chrono>

const int ANGLE_0 = 5;  // 대략 0도
const int ANGLE_45 = 9;
const int ANGLE_90 = 15; // 대략 90도
const int ANGLE_135 = 19;
const int ANGLE_180 = 25; // 대략 180도

ServoManager::ServoManager(int tl, int tr, int bl, int br)
    : top_left_pin(tl), top_right_pin(tr), bottom_left_pin(bl), bottom_right_pin(br) {}


void ServoManager::init() {
    // softPwmCreate(핀번호, 초기값, PWM Range(200 = 20ms 주기))
    softPwmCreate(top_left_pin, 0, 200);
    softPwmCreate(top_right_pin, 0, 200);
    softPwmCreate(bottom_left_pin, 0, 200);
    softPwmCreate(bottom_right_pin, 0, 200);
    reset(); // 초기 상태로 세팅
    std::cout<<"[servo] Initialized\n";
}

void ServoManager::set_angle(int pin, int pwm_value) {
    softPwmWrite(pin, pwm_value);
}

void ServoManager::close_lid() {
    std::cout << "[Servo] 뚜껑 닫기\n";
    set_angle(top_left_pin, ANGLE_135); 
    std::this_thread::sleep_for(std::chrono::milliseconds(SERVO_DELAY));
    set_angle(top_left_pin, 0); // 모터 전력 차단 (파닥거림 방지)

    set_angle(top_right_pin, ANGLE_135);
    std::this_thread::sleep_for(std::chrono::milliseconds(SERVO_DELAY));
    set_angle(top_right_pin, 0); // 모터 전력 차단 (파닥거림 방지)
}

void ServoManager::open_lid() {
    std::cout << "[Servo] 뚜껑 열기\n";
    set_angle(top_left_pin, ANGLE_45);
    std::this_thread::sleep_for(std::chrono::milliseconds(SERVO_DELAY));
    set_angle(top_left_pin, 0); 

    set_angle(top_right_pin, ANGLE_45);
    std::this_thread::sleep_for(std::chrono::milliseconds(SERVO_DELAY));
    set_angle(top_right_pin, 0); 
}

void ServoManager::open_bottom() {
    std::cout << "[Servo] 바닥 열기\n";
    set_angle(bottom_left_pin, ANGLE_45);
    std::this_thread::sleep_for(std::chrono::milliseconds(SERVO_DELAY));
    set_angle(bottom_left_pin, 0); 

    set_angle(bottom_right_pin, ANGLE_45);
    std::this_thread::sleep_for(std::chrono::milliseconds(SERVO_DELAY));
    set_angle(bottom_right_pin, 0); 
}

void ServoManager::close_bottom() {
    std::cout << "[Servo] 바닥 닫기\n";
    set_angle(bottom_left_pin, ANGLE_135);
    std::this_thread::sleep_for(std::chrono::milliseconds(SERVO_DELAY));
    set_angle(bottom_left_pin, 0); 

    set_angle(bottom_right_pin, ANGLE_135);
    std::this_thread::sleep_for(std::chrono::milliseconds(SERVO_DELAY));
    set_angle(bottom_right_pin, 0); 
}

void ServoManager::reset() {
    std::cout << "[Servo] 시스템 초기화\n";
    open_lid();
    close_bottom();
}