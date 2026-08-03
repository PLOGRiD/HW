#include <iostream>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <wiringPi.h>

#include "config.h"
#include "common.h"
#include "event.h"
#include "network.h"

#include "sensor/camera.h"
#include "sensor/ultrasonic.h"
#include "sensor/spectroscopy.h"
#include "sensor/ledStrip.h"
#include "sensor/gps.h"

std::queue<PloggingData> uploadQueue;
std::mutex queueMutex;
GpsData globalGpsData;
std::mutex gpsMutex;

int main() {
    if (wiringPiSetup() == -1) {
        std::cerr << "[wiringPi] Error: set gpio\n";
        return -1;
    }


    // main

    // SpectroscopySensor spectro;
    // Camera cam;
    // LedStrip led(LED_PIN);
    // Ultrasonic ultrasonic_1(TRIG_PIN_1, ECHO_PIN_1);
    // Ultrasonic ultrasonic_2(TRIG_PIN_2, ECHO_PIN_2);
    // GpsSensor gps(GPS_SERIAL_PORT);
    // ServoManager servo(SERVO_TOP_LEFT, SERVO_TOP_RIGHT, SERVO_BOTTOM_LEFT, SERVO_BOTTOM_RIGHT);

    // // init
    // spectro.init();
    // cam.init();
    // led.init();
    // ultrasonic_1.init();
    // ultrasonic_2.init();
    // ultrasonic_1.set_base();
    // std::this_thread::sleep_for(std::chrono::milliseconds(DETECT_DELAY));
    // ultrasonic_2.set_base();
    // servo.init();
    // std::cout<<"\n[System] Initialized\n";

    // // thread
    // std::thread eventThread(event_processing_thread, std::ref(ultrasonic_1), std::ref(ultrasonic_2), std::ref(cam), std::ref(spectro), std::ref(led), std::ref(servo));
    // std::thread httpThread(http_transmission_thread);

    // eventThread.join();
    // httpThread.join();


    // 테스트 및 데이터 수집용 임시 코드
    while(true){
        std::cout<<"이용할 서비스 번호를 입력하세요\n1: 데이터 수집\n2:서버 연동 AI 테스트\n";
        int dummy;
        std::cin>>dummy;
        if(dummy==1){
            // 데이터 수집
            SpectroscopySensor spectro;
            Camera cam;
            LedStrip led(LED_PIN);

            spectro.init();
            cam.init();
            led.init();
    
            std::thread eventThread(event_for_AI_Thread, std::ref(cam), std::ref(spectro), std::ref(led));
            eventThread.join();
        }
        else if(dummy==2){
            // 서버 연동 AI 테스트
            SpectroscopySensor spectro;
            Camera cam;
            LedStrip led(LED_PIN);
            GpsSensor gps(GPS_SERIAL_PORT);

            spectro.init();
            cam.init();
            led.init();

            std::thread eventThread(event_for_test_Thread,  std::ref(cam), std::ref(spectro), std::ref(led));
            std::thread httpThread(http_transmission_thread);
            std::thread gpsThread(gps_thread);

            eventThread.join();
            httpThread.join();
            gpsThread.join();
        }
        else{
            std::cout<<"잘못된 입력입니다. 다시 입력하세요\n";
        }
    }

    return 0;
}
