#include "sensor/servo.h"
#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <thread>
#include <chrono>

const int ANGLE_0 = 5;  // 대략 0도
const int ANGLE_90 = 15; // 대략 90도
const int ANGLE_180 = 25; // 대략 180도

ServoManager::ServoManager(int tl, int tr, int bl, int br)
    : top_left_pin(tl), top_right_pin(tr), bottom_left_pin(bl), bottom_right_pin(br) {}

void ServoManager::init() {
    // softPwmCreate(핀번호, 초기값, PWM Range(200 = 20ms 주기))
    softPwmCreate(top_left_pin, 0, 200);
    softPwmCreate(top_right_pin, 0, 200);
    softPwmCreate(bottom_left_pin, 0, 200);
    softPwmCreate(bottom_right_pin, 0, 200);
    reset(); // 부팅 시 안전하게 초기 위치로 세팅
    std::cout<<"[servo] Initialized\n";
}

void ServoManager::set_angle(int pin, int pwm_value) {
    softPwmWrite(pin, pwm_value);
}

void ServoManager::close_lid() {
    std::cout << "[Servo] 뚜껑 닫기 (암실 조성)\n";
    // 🚨 팁: 양쪽 문이 마주 보고 달린 경우, 
    // 하나는 정방향, 하나는 역방향으로 돌아가야 문이 닫힙니다.
    set_angle(top_left_pin, ANGLE_90); 
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    set_angle(top_right_pin, ANGLE_0); // 반대 방향
    
    // 모터가 목표 위치까지 돌아갈 시간을 잠시 벌어줍니다.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void ServoManager::open_lid() {
    std::cout << "[Servo] 뚜껑 열기\n";
    set_angle(top_left_pin, ANGLE_0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    set_angle(top_right_pin, ANGLE_90);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void ServoManager::open_bottom() {
    std::cout << "[Servo] 바닥 열기 (객체 배출)\n";
    set_angle(bottom_left_pin, ANGLE_90);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    set_angle(bottom_right_pin, ANGLE_0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void ServoManager::close_bottom() {
    std::cout << "[Servo] 바닥 닫기\n";
    set_angle(bottom_left_pin, ANGLE_0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    set_angle(bottom_right_pin, ANGLE_90);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void ServoManager::reset() {
    std::cout << "[Servo] 전체 시스템 초기 위치로 복구\n";
    open_lid();     // 다음 투입을 위해 뚜껑 개방
    close_bottom(); // 쓰레기가 안 빠지도록 바닥 폐쇄
}