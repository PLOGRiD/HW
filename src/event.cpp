#include "event.h"
#include "common.h"
#include "config.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <string>

void event_processing_thread(Ultrasonic& ultrasonic, Camera& cam, SpectroscopySensor& spec, LedStrip& led) {
    std::cout << "[이벤트 스레드]\n";

    // // 분광센서 전처리용 데이터 수집
    // std::cout<<"\nclose box\n";
    // std::string dummy;
    // std::getline(std::cin, dummy);
    // spec.readAllChannels();
    // std::cout<<"\nclose box with white paper\n";
    // std::getline(std::cin, dummy);
    // spec.readAllChannelsWithBulb();

    // std::cout<<"\nfinish to get reference\n";
    // std::getline(std::cin, dummy);

    // 메인로직 시작
    while (true) {
        while(true){
            double dist = ultrasonic.get_distance();

            if(ultrasonic.detect_event(dist)){
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(DETECT_DELAY));
        } 
        
        std::cout << "\n[eventThread] 쓰레기 투입 감지\n";
        std::cout << "\n[eventThread] 뚜껑을 닫은 후 엔터를 누르세요\n";

        std::string dummy;
        std::getline(std::cin, dummy);

        std::cout << "\n[eventThread] 데이터 수집 시작\n";
        PloggingData currentData;

        // 이미지 촬영
        led.on();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        currentData.image_path = cam.capture();
        led.off();

        if(currentData.image_path.empty()){
            std::cout << "[Camera] capture failed\n";
            continue;
        }

        // // 분광센서
        // currentData.spec_data = spec.normalizeAllChannels(); 

        // 큐에 적재
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            uploadQueue.push(currentData);
            std::cout << "[eventThread] 현재 전송 대기열: " << uploadQueue.size() << "개\n";
        }

        std::cout << "\n[eventThread] 쓰레기 제거 후 엔터를 누르세요\n";
        std::getline(std::cin, dummy);
    }
}