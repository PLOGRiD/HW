#include "event.h"
#include "common.h"
#include "config.h"
#include "sensor/spectroscopy.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <string>


void event_processing_thread(Ultrasonic& ultrasonic_1, Ultrasonic& ultrasonic_2, Camera& cam, SpectroscopySensor& spec, LedStrip& led, ServoManager& servo) {
    std::cout << "[이벤트 스레드]\n";

    // 메인로직 시작
    while (true) {
        std::cout << "\n[eventThread] 쓰레기 투입 감지 대기\n";
        while(true){
            double dist = ultrasonic_1.get_distance();
            if(ultrasonic_1.detect_event(dist)){
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(DETECT_DELAY));

            dist = ultrasonic_2.get_distance();
            if(ultrasonic_2.detect_event(dist)){
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(DETECT_DELAY));
        } 
        
        std::cout << "\n[eventThread] 쓰레기 투입 감지\n";

        PloggingData currentData;

        // gps 데이터 복사
        {
            std::lock_guard<std::mutex> lock(gpsMutex);
            currentData.gps_location = globalGpsData; 
            std::cout << "[eventThread] GPS 데이터 복사 완료\n";
        }

        std::cout << "[eventThread] 투입구 폐쇄 완료\n";
        servo.close_lid();

        // temp
        std::string dummy;
        std::getline(std::cin, dummy);

        std::cout << "[eventThread] 이미지 촬영 시작\n";

        // 이미지 촬영
        led.on();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        currentData.image_path = cam.capture();
        led.off();

        if(currentData.image_path.empty()){
            std::cout << "[Camera] capture failed\n";
            continue;
        }

        // 분광센서
        std::cout << "[eventThread] 분광 데이터 수집 시작\n";
        currentData.spec_data = spec.normalize_all_channels(); 

        // 큐에 적재
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            uploadQueue.push(currentData);
            std::cout << "[eventThread] 큐 적재 완료\n";
            std::cout<<"[eventThread] 현재 전송 대기열: " << uploadQueue.size() << "개\n";
        }

        std::cout << "[eventThread] 쓰레기 배출 시작\n";
        servo.open_bottom();
        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); 


        std::cout << "[eventThread] 시스템 리셋 시작\n";
        servo.reset(); 

        // temp
        std::cout<<"[eventThread] finish reset\n";
        std::getline(std::cin, dummy);
    }
}


void event_for_AI_Thread(Camera& cam, SpectroscopySensor& spec, LedStrip& led) {
    std::cout << "[eventThread] 레퍼런스 측정 시작\n";
    spec.calibrate_references();
    std::cout << "[eventThread] 레퍼런스 측정 완료\n";

    // 메인로직 시작
    while (true) {
        std::cout << "[eventThread] 쓰레기 투입 후 엔터를 누르세요\n";

        std::string dummy;
        std::getline(std::cin, dummy);

        std::cout << "[eventThread] 이미지 촬영 시작\n";

        // 이미지 촬영
        led.on();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        std::string image_path="";
        image_path = cam.capture();
        led.off();

        if(image_path==""){
            std::cout << "[Camera] capture failed\n";
            continue;
        }

        // 분광센서
        std::cout << "[eventThread] 분광 데이터 수집 시작\n";
        spec.collect_data_for_AI();

        std::cout<<"[eventThread] 분광 데이터 수집 완료\n";
    }
}