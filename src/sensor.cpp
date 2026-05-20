#include "sensor.h"
#include "SparkFun_AS7265X.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <wiringPi.h>
#include <string>

using namespace std;

SpectroscopySensor::SpectroscopySensor(){
    file_name = "trash_spectroscopy_data_" + to_string(time(nullptr)) + ".csv";
    labels = {
        {1, "PET"},
        {2, "glass"},
        {0, "END"}
    };
}

bool SpectroscopySensor::init(){
    if (!as7265x.begin()){
        std::cout<<"[AS7265X Sensor Error] Not found\n";
        return false;
    }
    return true;
}

SpectroscopyData SpectroscopySensor::readAllChannels(){
    SpectroscopyData data;

    as7265x.takeMeasurements(); 
    delay(100);

    data.A = as7265x.getCalibratedA();
    data.B = as7265x.getCalibratedB();
    data.C = as7265x.getCalibratedC();
    data.D = as7265x.getCalibratedD();
    data.E = as7265x.getCalibratedE();
    data.F = as7265x.getCalibratedF();
    data.G = as7265x.getCalibratedG();
    data.H = as7265x.getCalibratedH();
    data.I = as7265x.getCalibratedI();
    data.J = as7265x.getCalibratedJ();
    data.K = as7265x.getCalibratedK();
    data.L = as7265x.getCalibratedL();
    data.R = as7265x.getCalibratedR();
    data.S = as7265x.getCalibratedS();
    data.T = as7265x.getCalibratedT();
    data.U = as7265x.getCalibratedU();
    data.V = as7265x.getCalibratedV();
    data.W = as7265x.getCalibratedW();

    return data;
}

SpectroscopyData SpectroscopySensor::readAllChannelsWithBulb(){
    SpectroscopyData data;

    as7265x.enableBulb(AS7265x_LED_WHITE);
    as7265x.enableBulb(AS7265x_LED_IR);
    as7265x.enableBulb(AS7265x_LED_UV);
    delay(100);
    data = readAllChannels();
    as7265x.disableBulb(AS7265x_LED_WHITE);
    as7265x.disableBulb(AS7265x_LED_IR);
    as7265x.disableBulb(AS7265x_LED_UV);

    return data;
}

float SpectroscopySensor::calcReflectance(float raw, float dark, float white){
    float diff = white - dark;
    if (diff <= 0.0001f && diff >= -0.0001f) {
        diff = 0.0001f; // 분모 0 방지
    }
    float value = (raw - dark) / diff;

    return value > 0.0f ? value : 0.0f;
}

void SpectroscopySensor::calibrateReferences() {
    cout << "---------------------------\n";
    
    cout << "measure noise\n";
    cout << "enter with empty box\n";
    cin.ignore();
    cin.get();
    
    cout << "wait...\n";
    dark_ref = readAllChannels();
    cout << "finish\n\n";

    cout << "---------------------------\n";
    cout << "measure white reference\n";
    cout << "enter with white paper\n";
    cin.get();
    
    cout << "wait...\n";
    white_ref = readAllChannelsWithBulb();
    cout << "finish\n";
    cout << "---------------------------\n\n";
}

SpectroscopyData SpectroscopySensor::normalizeAllChannels(){
    SpectroscopyData raw_data = readAllChannelsWithBulb();
    SpectroscopyData cal_data;

    cal_data.A = calcReflectance(raw_data.A, dark_ref.A, white_ref.A);
    cal_data.B = calcReflectance(raw_data.B, dark_ref.B, white_ref.B);
    cal_data.C = calcReflectance(raw_data.C, dark_ref.C, white_ref.C);
    cal_data.D = calcReflectance(raw_data.D, dark_ref.D, white_ref.D);
    cal_data.E = calcReflectance(raw_data.E, dark_ref.E, white_ref.E);
    cal_data.F = calcReflectance(raw_data.F, dark_ref.F, white_ref.F);
    cal_data.G = calcReflectance(raw_data.G, dark_ref.G, white_ref.G);
    cal_data.H = calcReflectance(raw_data.H, dark_ref.H, white_ref.H);
    cal_data.I = calcReflectance(raw_data.I, dark_ref.I, white_ref.I);
    cal_data.J = calcReflectance(raw_data.J, dark_ref.J, white_ref.J);
    cal_data.K = calcReflectance(raw_data.K, dark_ref.K, white_ref.K);
    cal_data.L = calcReflectance(raw_data.L, dark_ref.L, white_ref.L);
    cal_data.R = calcReflectance(raw_data.R, dark_ref.R, white_ref.R);
    cal_data.S = calcReflectance(raw_data.S, dark_ref.S, white_ref.S);
    cal_data.T = calcReflectance(raw_data.T, dark_ref.T, white_ref.T);
    cal_data.U = calcReflectance(raw_data.U, dark_ref.U, white_ref.U);
    cal_data.V = calcReflectance(raw_data.V, dark_ref.V, white_ref.V);
    cal_data.W = calcReflectance(raw_data.W, dark_ref.W, white_ref.W);

    float sum = cal_data.A + cal_data.B + cal_data.C + cal_data.D + cal_data.E + cal_data.F + 
                cal_data.G + cal_data.H + cal_data.I + cal_data.J + cal_data.K + cal_data.L + 
                cal_data.R + cal_data.S + cal_data.T + cal_data.U + cal_data.V + cal_data.W;

    if (sum > 0.0f) {
        cal_data.A /= sum; cal_data.B /= sum; cal_data.C /= sum;
        cal_data.D /= sum; cal_data.E /= sum; cal_data.F /= sum;
        cal_data.G /= sum; cal_data.H /= sum; cal_data.I /= sum;
        cal_data.J /= sum; cal_data.K /= sum; cal_data.L /= sum;
        cal_data.R /= sum; cal_data.S /= sum; cal_data.T /= sum;
        cal_data.U /= sum; cal_data.V /= sum; cal_data.W /= sum;
    }

    return cal_data;
}

void SpectroscopySensor::collectDataForAI(){
    ofstream csv_file;
    csv_file.open(file_name, ios::out | ios::app);

    csv_file.seekp(0, ios::end);
    if(csv_file.tellp() == 0){
        csv_file<<"Label,A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W,note\n";
    }

    calibrateReferences();

    int user_input =-1;
    while(true){
        cout<<"---------------------------\n";
        for(const auto& pair:labels){
            cout<<"["<<pair.first<<"] "<< pair.second<<"\n";
        }
        cout<<"---------------------------\n";
        cout<<"write label num\n";
        
        cin>>user_input;

        if(labels.find(user_input) == labels.end()){
            cout<<"wrong num, retry\n";
            continue;
        }

        if (user_input == 0){
            cout<<"end\n";
            break;
        }

        string note = " ";
        cout << "memo : ";
        cin >> note;

        cout << "wait...\n";

        SpectroscopyData cal_data = normalizeAllChannels();

        csv_file << labels[user_input] << "," 
                << cal_data.A << ","<< cal_data.B << ","<< cal_data.C << ","<< cal_data.D << ","<< cal_data.E << ","
                << cal_data.F << ","<< cal_data.G << ","<< cal_data.H << ","<< cal_data.I << ","<< cal_data.J << ","
                << cal_data.K << ","<< cal_data.L << ","<< cal_data.R << ","<< cal_data.S << ","<< cal_data.T << ","
                << cal_data.U << ","<< cal_data.V << ","<< cal_data.W << ","
                << note << "\n";

        cout<<"finish\n";

    }

    csv_file.close();
}