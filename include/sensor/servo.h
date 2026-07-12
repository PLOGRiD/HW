#pragma once

class ServoManager {
private:
    int top_left_pin;
    int top_right_pin;
    int bottom_left_pin;
    int bottom_right_pin;

    // 내부에서만 사용하는 개별 모터 각도 제어 함수
    void set_angle(int pin, int pwm_value);

public:
    // 생성자: 4개의 핀 번호를 받아서 초기화
    ServoManager(int tl, int tr, int bl, int br);
    
    void init();
    
    // 뚜껑 (투입구) 제어
    void close_lid();   // 뚜껑 닫기 (암실 조성)
    void open_lid();    // 뚜껑 열기 (기본 상태)
    
    // 바닥 (배출구) 제어
    void open_bottom(); // 바닥 열기 (쓰레기 배출)
    void close_bottom();// 바닥 닫기 (기본 상태)
    
    // 시스템 초기화
    void reset();       // 뚜껑은 열고, 바닥은 닫은 기본 상태로 복구
};