#ifndef __SENSOR_H__
#define __SENSOR_H__

class Ultrasonic {
private:
    int trig_pin;
    int echo_pin;

public:
    Ultrasonic(int trig, int echo);

    void init_sensor(); // 초기화
    double get_distance(); // 거리 측정
    void test_work(); // 작동 테스트용
};

#endif