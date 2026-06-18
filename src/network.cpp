#include "network.h"
#include "common.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <cstdio>

void httpTransmissionThread() {

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

                curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.0.x:8080/api/upload"); // 임시주소

                form = curl_mime_init(curl);

                field = curl_mime_addpart(form);
                curl_mime_name(field, "image");
                curl_mime_filedata(field, dataToSend.image_path.c_str());

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

                curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

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

                    if (std::remove(dataToSend.image_path.c_str()) == 0) {
                    } else {
                        std::cerr << "[HTTP] 이미지 삭제 실패\n";
                    }
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