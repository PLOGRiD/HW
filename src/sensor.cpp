#include "sensor.h"
#include "config.h"
#include "SparkFun_AS7265X.h"
#include <wiringPi.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>

using namespace std;

// 초음파센서 ---------------------------------------------------------------------

Ultrasonic::Ultrasonic(int trig, int echo)
    : trig_pin(trig), echo_pin(echo), base_distance(0), count(0) {}
    

void Ultrasonic::init_sensor() {
    pinMode(trig_pin, OUTPUT);
    pinMode(echo_pin, INPUT);

    digitalWrite(trig_pin, LOW);
}

double Ultrasonic::get_distance() {
    digitalWrite(trig_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_pin, LOW);

    unsigned int start = micros();
    while (digitalRead(echo_pin) == LOW) {
        if (micros() - start > 30000) return -1;
    }

    start = micros();
    while (digitalRead(echo_pin) == HIGH) {
        if (micros() - start > 30000) return -1;
    }
    unsigned int time = micros() - start;

    double distance = time / 58.0;

    return distance;
}

void Ultrasonic::set_base(){
    double sum = 0;
    double distance;

    for(int i = 0; i < 10; i++){
        distance = get_distance();
        if (distance < 0){
            i--;
            continue;
        }
        sum += distance;
        delay(50);
    }

    base_distance = sum / 10.0;
}

bool Ultrasonic::detect_event(double distance){
    if (distance < 0){
        cerr<<"get_distance Error\n";
        return false;
    }

    bool isChanged = (distance < base_distance - DETECT_GAP);

    if (isChanged) {
        count++;
        if (count >= DETECT_COUNT) {
            count = 0;
            return true;
        }
    } else {
        count = 0;
    }

    return false;
}

void Ultrasonic::test_work(int delay_time){
    
    while(true){
        cout << "[test_ultrasonic] distance: " << get_distance() << "\n";
        delay(delay_time);
    }
};

// 분광센서 ---------------------------------------------------------------------

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
        cout<<"[AS7265X Sensor Error] Not found\n";
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

    cout<< dark_ref.A << ","<< dark_ref.B << ","<< dark_ref.C << ","<< dark_ref.D << ","<< dark_ref.E << ","
                << dark_ref.F << ","<< dark_ref.G << ","<< dark_ref.H << ","<< dark_ref.I << ","<< dark_ref.J << ","
                << dark_ref.K << ","<< dark_ref.L << ","<< dark_ref.R << ","<< dark_ref.S << ","<< dark_ref.T << ","
                << dark_ref.U << ","<< dark_ref.V << ","<< dark_ref.W << "\n";


    cout << "---------------------------\n";
    cout << "measure white reference\n";
    cout << "enter with white paper\n";
    cin.get();
    
    cout << "wait...\n";
    white_ref = readAllChannelsWithBulb();
    cout << "finish\n";
    cout << "---------------------------\n\n";

    cout<< white_ref.A << ","<< white_ref.B << ","<< white_ref.C << ","<< white_ref.D << ","<< white_ref.E << ","
                << white_ref.F << ","<< white_ref.G << ","<< white_ref.H << ","<< white_ref.I << ","<< white_ref.J << ","
                << white_ref.K << ","<< white_ref.L << ","<< white_ref.R << ","<< white_ref.S << ","<< white_ref.T << ","
                << white_ref.U << ","<< white_ref.V << ","<< white_ref.W << "\n";
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


// led 스트립 ---------------------------------------------------------------------

LedStrip::LedStrip(int pin) : pin(pin) {}

void LedStrip::init() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void LedStrip::on() {
    digitalWrite(pin, HIGH);
}

void LedStrip::off() {
    digitalWrite(pin, LOW);
}