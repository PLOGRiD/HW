#include "sensor/spectroscopy.h"
#include "SparkFun_AS7265X.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <wiringPi.h>

using namespace std;

SpectroscopySensor::SpectroscopySensor(){
    file_name = "trash_spectroscopy_data_" + to_string(time(nullptr)) + ".csv";
    labels = {
        {1, "PET"},
        {2, "glass"}
    };
}

bool SpectroscopySensor::init(){
    if (!as7265x.begin()){
        cout<<"[AS7265X Sensor Error] Not found\n";
        return false;
    }
    as7265x.disableIndicator();
    cout<<"[AS7265X] Initialized\n";

    return true;
}

SpectroscopyData SpectroscopySensor::read_all_channels(){
    SpectroscopyData data;

    as7265x.takeMeasurements(); 
    delay(100);

    data.channel[0] = as7265x.getCalibratedA();
    data.channel[1] = as7265x.getCalibratedB();
    data.channel[2] = as7265x.getCalibratedC();
    data.channel[3] = as7265x.getCalibratedD();
    data.channel[4] = as7265x.getCalibratedE();
    data.channel[5] = as7265x.getCalibratedF();
    data.channel[6] = as7265x.getCalibratedG();
    data.channel[7] = as7265x.getCalibratedH();
    data.channel[8] = as7265x.getCalibratedI();
    data.channel[9] = as7265x.getCalibratedJ();
    data.channel[10] = as7265x.getCalibratedK();
    data.channel[11] = as7265x.getCalibratedL();
    data.channel[12] = as7265x.getCalibratedR();
    data.channel[13] = as7265x.getCalibratedS();
    data.channel[14] = as7265x.getCalibratedT();
    data.channel[15] = as7265x.getCalibratedU();
    data.channel[16] = as7265x.getCalibratedV();
    data.channel[17] = as7265x.getCalibratedW();

    return data;
}

SpectroscopyData SpectroscopySensor::read_all_channels_with_bulb(){
    SpectroscopyData data;

    as7265x.enableBulb(AS7265x_LED_WHITE);
    as7265x.enableBulb(AS7265x_LED_IR);
    as7265x.enableBulb(AS7265x_LED_UV);
    delay(100);
    data = read_all_channels();
    as7265x.disableBulb(AS7265x_LED_WHITE);
    as7265x.disableBulb(AS7265x_LED_IR);
    as7265x.disableBulb(AS7265x_LED_UV);

    return data;
}

float SpectroscopySensor::calc_reflectance(float raw, float dark, float white){
    float diff = white - dark;
    if (diff <= 0.0001f && diff >= -0.0001f) {
        diff = 0.0001f; // 분모 0 방지
    }
    float value = (raw - dark) / diff;

    return value > 0.0f ? value : 0.0f;
}

void SpectroscopySensor::calibrate_references() {
    string dummy;
    std::ofstream csv_dark, csv_white;

    csv_dark.open("dark_reference", std::ios::out | std::ios::app);
    csv_dark.seekp(0, std::ios::end);
    if(csv_dark.tellp() == 0){
        csv_dark<<"A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W\n";
    }

    csv_white.open("white_reference", std::ios::out | std::ios::app);
    csv_white.seekp(0, std::ios::end);
    if(csv_white.tellp() == 0){
        csv_white<<"A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W\n";
    }

    cout << "[get_reference] 박스를 비우고 엔터를 누르세요\n";
    getline(std::cin, dummy);

    dark_ref = read_all_channels();
    for(int i=0;i<18;i++){
        csv_dark << dark_ref.channel[i] << ",";
    }
    csv_dark << "\n";
    cout << "[get_reference] 측정 완료\n";

    cout << "[get_reference] 흰 종이를 넣고 엔터를 누르세요\n";
    getline(std::cin, dummy);
    
    white_ref = read_all_channels_with_bulb();
    for(int i=0;i<18;i++){
        csv_white << white_ref.channel[i] << ",";
    }
    csv_white << "\n";
    cout << "[get_reference] 측정 완료\n";

    csv_dark.close();
    csv_white.close();
}

SpectroscopyData SpectroscopySensor::normalize_all_channels(){
    SpectroscopyData raw_data = read_all_channels_with_bulb();
    SpectroscopyData cal_data;
    float sum = 0;

    for(int i = 0; i < 18; i++){
        cal_data.channel[i] = calc_reflectance(raw_data.channel[i], dark_ref.channel[i], white_ref.channel[i]);
    }

    for(int i = 0; i < 18; i++){
        sum += cal_data.channel[i];
    }

    if (sum > 0.0f) {
        for(int i = 0; i < 18; i++){
            cal_data.channel[i] /= sum;
        }
    }

    return cal_data;
}

void SpectroscopySensor::collect_data_for_AI(){
    ofstream csv_file;
    csv_file.open(file_name, ios::out | ios::app);

    csv_file.seekp(0, ios::end);
    if(csv_file.tellp() == 0){
        csv_file<<"Label,A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W,note\n";
    }

    calibrate_references();

    int user_input =-1;
    while(true){

        for(const auto& pair:labels){
            cout<<"["<<pair.first<<"] "<< pair.second<<"\n";
        }

        cout<<"[Spectroscopy] 라벨 번호를 쓰세요\n";
        
        cin>>user_input;

        if(labels.find(user_input) == labels.end()){
            cout<<"[Spectroscopy] 잘못된 번호입니다 다시 입력하세요\n";
            continue;
        }

        string note = " ";
        cout << "메모 : ";
        cin >> note;

        SpectroscopyData cal_data = normalize_all_channels();

        csv_file << labels[user_input] << ",";
        for(int i=0;i<18;i++){
            csv_file << cal_data.channel[i] << ",";
        }
        csv_file << note << "\n";
        break;
    }
    csv_file.close();
}