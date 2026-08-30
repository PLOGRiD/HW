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
    curl_global_init(CURL_GLOBAL_ALL);

    const char* HTTP_API_URL = "https://plogrid.p-e.kr/api/v1/trashes/waste-classification";
    std::cout<<"\n[http thread] init\n";
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

                curl_easy_setopt(curl, CURLOPT_URL, HTTP_API_URL); 

                form = curl_mime_init(curl);

                // 1. 이미지 파일 추가
                field = curl_mime_addpart(form);
                curl_mime_name(field, "image");
                curl_mime_filedata(field, dataToSend.image_path.c_str());

                // 폼 데이터에 string 값을 추가하는 람다 함수
                auto add_string_field = [&](const char* name, const std::string& value) {
                    curl_mimepart* part = curl_mime_addpart(form);
                    curl_mime_name(part, name);
                    curl_mime_data(part, value.c_str(), CURL_ZERO_TERMINATED);
                };

                // 폼 데이터에 double 값을 추가하는 람다 함수
                auto add_double_field = [&](const char* name, double value) {
                    curl_mimepart* part = curl_mime_addpart(form);
                    curl_mime_name(part, name);
                    std::string val_str = std::to_string(value);
                    curl_mime_data(part, val_str.c_str(), CURL_ZERO_TERMINATED);
                };

                // 
                auto add_int64_field = [&](const char* name, int64_t value) {
                    curl_mimepart* part = curl_mime_addpart(form);
                    curl_mime_name(part, name);
                    std::string val_str = std::to_string(value);
                    curl_mime_data(part, val_str.c_str(), CURL_ZERO_TERMINATED);
                };

                // 디바이스 id 추가
                add_int64_field("deviceId", 1);

                // 2. 패키징된 GPS 및 타임스탬프 파라미터 추가 (dataToSend.gps_location 사용)
                add_string_field("timestamp", dataToSend.gps_location.timestamp);
                add_double_field("latitude", dataToSend.gps_location.latitude);
                add_double_field("longitude", dataToSend.gps_location.longitude);

                // 3. 패키징된 분광센서 데이터 파라미터 추가
                add_double_field("a", dataToSend.spec_data.channel[0]);
                add_double_field("b", dataToSend.spec_data.channel[1]);
                add_double_field("c", dataToSend.spec_data.channel[2]);
                add_double_field("d", dataToSend.spec_data.channel[3]);
                add_double_field("e", dataToSend.spec_data.channel[4]);
                add_double_field("f", dataToSend.spec_data.channel[5]);
                add_double_field("g", dataToSend.spec_data.channel[6]);
                add_double_field("h", dataToSend.spec_data.channel[7]);
                add_double_field("i", dataToSend.spec_data.channel[8]);
                add_double_field("j", dataToSend.spec_data.channel[9]);
                add_double_field("k", dataToSend.spec_data.channel[10]);
                add_double_field("l", dataToSend.spec_data.channel[11]);
                add_double_field("r", dataToSend.spec_data.channel[12]);
                add_double_field("s", dataToSend.spec_data.channel[13]);
                add_double_field("t", dataToSend.spec_data.channel[14]);
                add_double_field("u", dataToSend.spec_data.channel[15]);
                add_double_field("v", dataToSend.spec_data.channel[16]);
                add_double_field("w", dataToSend.spec_data.channel[17]);

                curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

                // 4. 서버로부터 탐지 결과(JSON)를 받기 위한 버퍼 설정
                std::string readBuffer;
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                // HTTP POST 요청 전송
                CURLcode res = curl_easy_perform(curl);
                
                if(res != CURLE_OK) {
                    std::cerr << "[HTTP] 전송 실패: " << curl_easy_strerror(res) << "\n";
                    // 실패 시 큐에 다시 집어넣어 재시도 (재포장 불필요)
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        uploadQueue.push(dataToSend);
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                } else {
                    std::cout << "[HTTP] 전송 완료\n";
                    std::cout << "[HTTP] 서버 응답: " << readBuffer << "\n";
                    
                    // 전송이 성공적으로 끝났을 때만 로컬 이미지 삭제
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
    std::cout<<"[MQTT] Initialized\n";
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