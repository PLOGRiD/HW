#pragma once

class Ultrasonic {
private:
    int trig_pin;
    int echo_pin;

    double base_distance;
    int count;

public:
    Ultrasonic(int trig, int echo);

    void init(); // 초기화
    void set_base(); // 기본 거리 설정
    double get_distance(); // 거리 측정
    bool detect_event(double distance); // 이벤트 감지
    void test_work(int delay_time); // 작동 테스트용
};