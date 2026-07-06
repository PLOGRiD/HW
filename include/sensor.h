#pragma once
#include <string>
#include <map>
#include <stdint.h>
#include <softPwm.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include "SparkFun_AS7265X.h"


// 분광센서 ---------------------------------------------------------------------

struct SpectroscopyData{
    float A, B, C, D, E, F;
    float G, H, I, J, K, L;
    float R, S, T, U, V, W; 
};

class SpectroscopySensor{
    private:
        AS7265X as7265x;
        std::map<int, std::string> labels;
        std::string file_name;
        SpectroscopyData dark_ref;
        SpectroscopyData white_ref;

        float calc_reflectance(float raw, float dark, float white);

    public:
        SpectroscopySensor();
        bool init();
        SpectroscopyData read_all_channels_with_bulb();
        SpectroscopyData read_all_channels();
        SpectroscopyData normalize_all_channels();
        void collect_data_for_AI();
        void calibrate_references();
};


// 초음파센서 ---------------------------------------------------------------------

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


// led 스트립 ---------------------------------------------------------------------

class LedStrip {
private:
    int pin;
public:
    LedStrip(int pin_num);
    
    void init();
    void on();
    void off();
};

// gps ---------------------------------------------------------------------

class GpsSensor {
private:
    int serial_fd;
    std::string port_name;

    bool parse_nmea(const std::string& nmea_line, double& lat, double& lon, std::string& time);

public:
    GpsSensor(std::string port);
    ~GpsSensor();
    void init();
    void update_gps(double& out_lat, double& out_lon, std::string& out_time);
};

void gps_thread(); 