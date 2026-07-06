#include "network.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <cstdio>
#include <string>

// 서버의 JSON 응답을 std::string으로 받기 위한 콜백 함수
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