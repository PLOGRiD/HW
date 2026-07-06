#include "sensor.h"
#include "config.h"
#include "SparkFun_AS7265X.h"
#include <wiringPi.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <softPwm.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <vector>
#include <sstream>
#include <cstring>


using namespace std;

// 초음파센서 ---------------------------------------------------------------------

Ultrasonic::Ultrasonic(int trig, int echo)
    : trig_pin(trig), echo_pin(echo), base_distance(0), count(0) {}
    

void Ultrasonic::init() {
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
        // if (count >= DETECT_COUNT) {
        //     count = 0;
        //     return true;
        // }
        return true;
    }
    // else {
    //     count = 0;
    // }

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
    as7265x.disableIndicator();
    
    return true;
}

SpectroscopyData SpectroscopySensor::read_all_channels(){
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
    cout << "---------------------------\n";
    
    cout << "measure noise\n";
    cout << "enter with empty box\n";
    cin.ignore();
    cin.get();
    
    cout << "wait...\n";
    dark_ref = read_all_channels();
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
    white_ref = read_all_channels_with_bulb();
    cout << "finish\n";
    cout << "---------------------------\n\n";

    cout<< white_ref.A << ","<< white_ref.B << ","<< white_ref.C << ","<< white_ref.D << ","<< white_ref.E << ","
                << white_ref.F << ","<< white_ref.G << ","<< white_ref.H << ","<< white_ref.I << ","<< white_ref.J << ","
                << white_ref.K << ","<< white_ref.L << ","<< white_ref.R << ","<< white_ref.S << ","<< white_ref.T << ","
                << white_ref.U << ","<< white_ref.V << ","<< white_ref.W << "\n";
}

SpectroscopyData SpectroscopySensor::normalize_all_channels(){
    SpectroscopyData raw_data = read_all_channels_with_bulb();
    SpectroscopyData cal_data;

    cal_data.A = calc_reflectance(raw_data.A, dark_ref.A, white_ref.A);
    cal_data.B = calc_reflectance(raw_data.B, dark_ref.B, white_ref.B);
    cal_data.C = calc_reflectance(raw_data.C, dark_ref.C, white_ref.C);
    cal_data.D = calc_reflectance(raw_data.D, dark_ref.D, white_ref.D);
    cal_data.E = calc_reflectance(raw_data.E, dark_ref.E, white_ref.E);
    cal_data.F = calc_reflectance(raw_data.F, dark_ref.F, white_ref.F);
    cal_data.G = calc_reflectance(raw_data.G, dark_ref.G, white_ref.G);
    cal_data.H = calc_reflectance(raw_data.H, dark_ref.H, white_ref.H);
    cal_data.I = calc_reflectance(raw_data.I, dark_ref.I, white_ref.I);
    cal_data.J = calc_reflectance(raw_data.J, dark_ref.J, white_ref.J);
    cal_data.K = calc_reflectance(raw_data.K, dark_ref.K, white_ref.K);
    cal_data.L = calc_reflectance(raw_data.L, dark_ref.L, white_ref.L);
    cal_data.R = calc_reflectance(raw_data.R, dark_ref.R, white_ref.R);
    cal_data.S = calc_reflectance(raw_data.S, dark_ref.S, white_ref.S);
    cal_data.T = calc_reflectance(raw_data.T, dark_ref.T, white_ref.T);
    cal_data.U = calc_reflectance(raw_data.U, dark_ref.U, white_ref.U);
    cal_data.V = calc_reflectance(raw_data.V, dark_ref.V, white_ref.V);
    cal_data.W = calc_reflectance(raw_data.W, dark_ref.W, white_ref.W);

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

        SpectroscopyData cal_data = normalize_all_channels();

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
    softPwmCreate(pin,0,40);
    // digitalWrite(pin, LOW);
}

void LedStrip::on() {
    digitalWrite(pin, HIGH);
    softPwmWrite(pin,(10/100.0)*40);
}

void LedStrip::off() {
    digitalWrite(pin, LOW);
}


// gps ---------------------------------------------------------------------


GpsSensor::GpsSensor(std::string port) : port_name(port), serial_fd(-1) {}

GpsSensor::~GpsSensor() {
    if (serial_fd != -1) {
        close(serial_fd);
    }
}

void GpsSensor::init() {
    serial_fd = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (serial_fd == -1) {
        std::cerr << "\n[Error] GPS 시리얼 포트를 열 수 없습니다: " << port_name << std::endl;
        return;
    }

    struct termios options;
    tcgetattr(serial_fd, &options);

    // 9600 bps 설정 (NEO-M8N의 기본 통신 속도)
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag |= (CLOCAL | CREAD); // 수신 활성화
    options.c_cflag &= ~PARENB;          // 패리티 비트 없음
    options.c_cflag &= ~CSTOPB;          // 1 스탑 비트
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;              // 8 데이터 비트

    tcsetattr(serial_fd, TCSANOW, &options);
    std::cout << "[Info] GPS 센서 초기화 완료 (" << port_name << ")" << std::endl;
}

// NMEA 문자열 파싱 (DDMM.MMMM -> DD.DDDD 변환 포함)
bool GpsSensor::parse_nmea(const std::string& nmea_line, double& lat, double& lon, std::string& time) {
    // $GPGGA 또는 $GNGGA 문장인지 확인
    if (nmea_line.find("$GPGGA") == 0 || nmea_line.find("$GNGGA") == 0) {
        std::stringstream ss(nmea_line);
        std::string token;
        std::vector<std::string> tokens;

        // 콤마(,)를 기준으로 문자열 분리
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        // GPGGA는 최소 15개의 필드가 있어야 함
        if (tokens.size() >= 10 && !tokens[2].empty() && !tokens[4].empty()) {
            time = tokens[1]; // UTC 시간

            // 위도 (Latitude) 파싱: DDMM.MMMM 형식
            double raw_lat = std::stod(tokens[2]);
            int lat_degrees = (int)(raw_lat / 100);
            double lat_minutes = raw_lat - (lat_degrees * 100);
            lat = lat_degrees + (lat_minutes / 60.0);
            if (tokens[3] == "S") lat = -lat; // 남반구면 마이너스

            // 경도 (Longitude) 파싱: DDDMM.MMMM 형식
            double raw_lon = std::stod(tokens[4]);
            int lon_degrees = (int)(raw_lon / 100);
            double lon_minutes = raw_lon - (lon_degrees * 100);
            lon = lon_degrees + (lon_minutes / 60.0);
            if (tokens[5] == "W") lon = -lon; // 서반구면 마이너스

            return true;
        }
    }
    return false;
}

// 센서에서 데이터를 읽어와서 파싱을 시도하는 함수
void GpsSensor::update_gps(double& out_lat, double& out_lon, std::string& out_time) {
    if (serial_fd == -1) return;

    char buffer[256];
    std::string line = "";
    bool data_updated = false;

    // 한 줄(Line) 단위로 읽어오기 위한 간단한 루프
    while (!data_updated) {
        char c;
        int n = read(serial_fd, &c, 1);
        if (n > 0) {
            if (c == '\n') {
                // 줄바꿈을 만나면 파싱 시도
                if (parse_nmea(line, out_lat, out_lon, out_time)) {
                    data_updated = true; // 파싱 성공시 루프 탈출
                }
                line = ""; // 다음 줄을 위해 초기화
            } else if (c != '\r') {
                line += c;
            }
        } else {
            // 읽을 데이터가 없으면 잠시 대기
            usleep(10000); 
        }
    }
}

void gps_thread(){
    // 1. 스레드가 시작될 때 GPS 센서 객체를 생성하고 초기화합니다.
    // 라즈베리파이4의 UART 포트 경로를 적어줍니다. 기본 하드웨어 시리얼은 보통 "/dev/ttyS0" 입니다.
    GpsSensor sensor("/dev/ttyS0"); 
    sensor.init();

    // 센서 데이터를 받아올 로컬 임시 변수들
    double parsed_lat = 0.0;
    double parsed_lon = 0.0;
    std::string parsed_time = "";

    while(true){
        // 2. GPS 센서로부터 새로운 데이터를 읽어옵니다. 
        // (update_gps 내부 루프 덕분에 유효한 NMEA 문장이 파싱될 때까지 블로킹됩니다)
        sensor.update_gps(parsed_lat, parsed_lon, parsed_time);

        // 3. 읽어온 데이터를 안전하게 전역 공유 변수에 업데이트합니다.
        {
            std::lock_guard<std::mutex> lock(gpsMutex);
            globalGpsData.latitude = parsed_lat;
            globalGpsData.longitude = parsed_lon;
            globalGpsData.timestamp = parsed_time; // 타임스탬프 구조체 멤버 변수 연동
            globalGpsData.isValid = true;
        }

        // 시리얼 데이터 수신 타이밍과 CPU 과점유 방지를 위해 아주 잠깐 쉬어줍니다.
        usleep(50000); // 50ms 대기
    }
}