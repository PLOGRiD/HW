#include "network.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <mosquitto.h> 


// mqtt 서버 설정 
const char* BROKER_ADDRESS = "127.0.0.1"; // 예: "192.168.0.10"
const int BROKER_PORT = 1883;
const char* TOPIC_NAME = "plogging/gps";

// http 서버의 JSON 응답을 std::string으로 받기 위한 콜백 함수
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void http_transmission_thread() {
    std::cout<<"\nhttp thread\n";
    curl_global_init(CURL_GLOBAL_ALL);

    while (true) {
        PloggingData dataToSend;
        bool hasData = false;

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!uploadQueue.empty()) {
                dataToSend = uploadQueue.front();
                uploadQueue.pop();
                hasData = true;
            }
        }

        if (hasData) {
            std::cout << "\n[HTTP] 전송 준비: " << dataToSend.image_path << "\n";
            
            CURL *curl = curl_easy_init();
            if(curl) {
                curl_mime *form = NULL;
                curl_mimepart *field = NULL;

                // 1. URL 변경 (백엔드 Predict API 포트 및 경로 반영)
                curl_easy_setopt(curl, CURLOPT_URL, "http://13.201.72.157:8000/predict"); 

                form = curl_mime_init(curl);

                // 2. 파일 필드 이름을 백엔드 명세서에 맞춰 "file"로 변경
                field = curl_mime_addpart(form);
                curl_mime_name(field, "file");
                curl_mime_filedata(field, dataToSend.image_path.c_str());

                // 3. 기존 분광센서(spectroscopy) 데이터 전송 부분 주석 처리
                /*
                std::string spec_json = "{\"A\":" + std::to_string(dataToSend.spec_data.A) + 
                                        ", \"B\":" + std::to_string(dataToSend.spec_data.B) +
                                        ", \"C\":" + std::to_string(dataToSend.spec_data.C) +
                                        ", \"D\":" + std::to_string(dataToSend.spec_data.D) +
                                        ", \"E\":" + std::to_string(dataToSend.spec_data.E) +
                                        ", \"F\":" + std::to_string(dataToSend.spec_data.F) +
                                        ", \"G\":" + std::to_string(dataToSend.spec_data.G) +
                                        ", \"H\":" + std::to_string(dataToSend.spec_data.H) +
                                        ", \"I\":" + std::to_string(dataToSend.spec_data.I) +
                                        ", \"J\":" + std::to_string(dataToSend.spec_data.J) +
                                        ", \"K\":" + std::to_string(dataToSend.spec_data.K) +
                                        ", \"L\":" + std::to_string(dataToSend.spec_data.L) +
                                        ", \"R\":" + std::to_string(dataToSend.spec_data.R) +
                                        ", \"S\":" + std::to_string(dataToSend.spec_data.S) +
                                        ", \"T\":" + std::to_string(dataToSend.spec_data.T) +
                                        ", \"U\":" + std::to_string(dataToSend.spec_data.U) +
                                        ", \"V\":" + std::to_string(dataToSend.spec_data.V) +
                                        ", \"W\":" + std::to_string(dataToSend.spec_data.W) +
                                        "}";
                
                field = curl_mime_addpart(form);
                curl_mime_name(field, "spectroscopy");
                curl_mime_data(field, spec_json.c_str(), CURL_ZERO_TERMINATED);
                */

                curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

                // 4. 서버로부터 탐지 결과(JSON)를 받기 위한 버퍼 설정
                std::string readBuffer;
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                // HTTP POST 요청 전송
                CURLcode res = curl_easy_perform(curl);
                
                if(res != CURLE_OK) {
                    std::cerr << "[HTTP] 전송 실패: " << curl_easy_strerror(res) << "\n";
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        uploadQueue.push(dataToSend);
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                } else {
                    std::cout << "[HTTP] 전송 완료\n";
                    
                    // 백엔드가 반환한 YOLO 탐지 결과 출력
                    std::cout << "[HTTP] 서버 응답: " << readBuffer << "\n";

                    // if (std::remove(dataToSend.image_path.c_str()) == 0) {
                    //     std::cout << "[HTTP] 이미지 삭제 완료\n";
                    // } else {
                    //     std::cerr << "[HTTP] 이미지 삭제 실패\n";
                    // }
                }

                curl_mime_free(form);
                curl_easy_cleanup(curl);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    curl_global_cleanup();
}


void mqtt_thread() {
    // 1. MQTT 라이브러리 초기화 및 클라이언트 생성
    mosquitto_lib_init();
    struct mosquitto *mosq = mosquitto_new("TrashCan_GPS_Client", true, NULL);

    if (!mosq) {
        std::cerr << "[Error] MQTT 클라이언트 생성 실패!" << std::endl;
        return;
    }

    // 2. 브로커 연결 시도
    int rc = mosquitto_connect(mosq, BROKER_ADDRESS, BROKER_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "[Error] MQTT 브로커 연결 실패. 네트워크 상태나 IP를 확인하세요." << std::endl;
        // 연결에 실패해도 스레드를 죽이지 않고 일단 넘어갑니다. (나중에 재연결 로직 추가 가능)
    } else {
        std::cout << "[Info] MQTT 브로커 연결 성공! (" << BROKER_ADDRESS << ")" << std::endl;
    }

    // MQTT 백그라운드 통신 루프 시작 (비동기 처리)
    mosquitto_loop_start(mosq);

    // 3. 무한 루프: 주기적으로 GPS 데이터 확인 후 전송
    while (true) {
        GpsData currentGps;

        // [중요] 다이어그램의 "GPS 데이터 복사" 부분
        // 뮤텍스로 잠그고 최대한 빠르게 데이터만 복사한 뒤 바로 풀어줍니다. (병목 현상 방지)
        {
            std::lock_guard<std::mutex> lock(gpsMutex);
            currentGps = globalGpsData;
        }

        // GPS 데이터가 유효할 때만 서버로 전송
        if (currentGps.isValid) {
            // 데이터를 JSON 형식의 문자열로 포장 (팀원들 서버가 파싱하기 좋게)
            std::string payload = "{\"latitude\": " + std::to_string(currentGps.latitude) +
                                  ", \"longitude\": " + std::to_string(currentGps.longitude) +
                                  ", \"timestamp\": \"" + currentGps.timestamp + "\"}";

            // MQTT Publish (발송)
            mosquitto_publish(mosq, NULL, TOPIC_NAME, payload.length(), payload.c_str(), 0, false);
            
            std::cout << "[MQTT 전송 완료] " << payload << std::endl;
        } else {
            std::cout << "[MQTT 대기] 유효한 GPS 데이터를 기다리는 중..." << std::endl;
        }

        // 전송 주기 설정 (예: 2초에 한 번씩 전송)
        // 너무 빨리 쏘면 서버가 과부하 걸리거나 라즈베리파이 CPU가 낭비됩니다.
        sleep(2); 
    }

    // 스레드 종료 시 자원 반납 (실제로는 무한루프라 도달하지 않음)
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}