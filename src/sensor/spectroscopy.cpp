#include "sensor/spectroscopy.h"
#include "sensor/servo.h"
#include "SparkFun_AS7265X.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <wiringPi.h>
#include <limits>
#include <filesystem>
#include <sstream>
#include <string>

using namespace std;

namespace {

// 레퍼런스 측정 시각 기반 세션 ID (예: 20261011_162715)
std::string make_session_id() {
    std::time_t now = std::time(nullptr);
    std::tm tm_now{};
    localtime_r(&now, &tm_now);

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm_now);
    return std::string(buf);
}

// 앞뒤 공백/개행(\r 포함) 제거
std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) {
        return "";
    }
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// 문자열 전체가 정수일 때만 true ("abc", "1abc", 빈 문자열, 너무 큰 수는 false)
bool parse_int(const std::string& s, int& out) {
    if (s.empty()) {
        return false;
    }
    try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) {
            return false;
        }
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

// CSV 한 칸에 안전하게 쓰기 (쉼표/따옴표/줄바꿈이 있으면 "..." 로 감싸고 " 는 "" 로)
std::string csv_escape(const std::string& s) {
    if (s.find_first_of(",\"\r\n") == std::string::npos) {
        return s;
    }
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out += c;
        }
    }
    out += "\"";
    return out;
}

}  // namespace

SpectroscopySensor::SpectroscopySensor(){
    labels = {
        {1, "PET"},
        {2, "glass"},
        {3, "paper"},
        {4, "vinyl"},
        {5, "can"}
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

bool SpectroscopySensor::load_references_from_file() {

    auto load_last_line = [](const std::string& filename,
                             SpectroscopyData& data,
                             std::string& session) -> bool {

        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "[Spectroscopy] 파일을 열 수 없습니다: "
                      << filename << "\n";
            return false;
        }

        std::string line;
        std::string last_line;

        // 파일의 마지막 비어있지 않은 줄 찾기
        while (std::getline(file, line)) {
            if (!line.empty()) {
                last_line = line;
            }
        }

        if (last_line.empty()) {
            std::cerr << "[Spectroscopy] 데이터가 없습니다: "
                      << filename << "\n";
            return false;
        }

        std::stringstream ss(last_line);
        std::string token;

        for (int i = 0; i < 18; i++) {
            if (!std::getline(ss, token, ',')) {
                std::cerr << "[Spectroscopy] 데이터 형식 오류: "
                          << filename << "\n";
                return false;
            }

            try {
                data.channel[i] = std::stof(token);
            }
            catch (...) {
                std::cerr << "[Spectroscopy] 숫자 변환 실패: "
                          << filename << "\n";
                return false;
            }
        }

        // 19번째 칸 = 세션 ID (세션 ID 추가 이전에 저장된 줄에는 없음 -> "legacy")
        session.clear();
        if (std::getline(ss, token, ',')) {
            session = trim(token);
        }
        if (session.empty()) {
            session = "legacy";
        }

        return true;
    };

    std::string dark_session, white_session;

    bool dark_ok = load_last_line(
        "./spectroscopy_datas/dark_reference.csv",
        dark_ref,
        dark_session
    );

    bool white_ok = load_last_line(
        "./spectroscopy_datas/white_reference.csv",
        white_ref,
        white_session
    );

    if (dark_ok && white_ok) {
        // dark/white 는 같은 세션에서 측정된 쌍이어야 함
        if (dark_session != white_session) {
            std::cerr << "[Spectroscopy] 레퍼런스 세션 ID 불일치 (dark: "
                      << dark_session << ", white: " << white_session
                      << ") - 레퍼런스를 다시 측정하세요\n";
            return false;
        }

        session_id = dark_session;
        std::cout << "[Spectroscopy] 마지막 레퍼런스 데이터 불러오기 완료 (session: "
                  << session_id << ")\n";
        return true;
    }

    std::cerr << "[Spectroscopy] 레퍼런스 데이터 로드 실패\n";
    return false;
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

void SpectroscopySensor::calibrate_references(ServoManager& servo) {
    string dummy;
    std::ofstream csv_dark, csv_white;

    std::filesystem::create_directory("./spectroscopy_datas");

    csv_dark.open("./spectroscopy_datas/dark_reference.csv", std::ios::out | std::ios::app);
    csv_dark.seekp(0, std::ios::end);
    if(csv_dark.tellp() == 0){
        csv_dark<<"A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W,session_id\n";
    }

    csv_white.open("./spectroscopy_datas/white_reference.csv", std::ios::out | std::ios::app);
    csv_white.seekp(0, std::ios::end);
    if(csv_white.tellp() == 0){
        csv_white<<"A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W,session_id\n";
    }

    getline(std::cin, dummy);
    cout << "[get_reference] 박스를 비우고 엔터를 누르세요\n";
    getline(std::cin, dummy);

    servo.close_lid();
    dark_ref = read_all_channels();
    cout << "[get_reference] 측정 완료\n";

    servo.open_lid();
    cout << "[get_reference] 흰 종이를 넣고 엔터를 누르세요\n";
    getline(std::cin, dummy);
    
    servo.close_lid();
    white_ref = read_all_channels_with_bulb();
    cout << "[get_reference] 측정 완료\n";
    servo.open_lid();

    // 두 측정이 모두 끝난 뒤 같은 세션 ID로 dark/white 를 함께 저장
    // (중간에 종료돼도 짝이 안 맞는 레퍼런스가 남지 않음)
    session_id = make_session_id();

    for(int i=0;i<18;i++){
        csv_dark << dark_ref.channel[i] << ",";
    }
    csv_dark << session_id << "\n";

    for(int i=0;i<18;i++){
        csv_white << white_ref.channel[i] << ",";
    }
    csv_white << session_id << "\n";

    csv_dark.close();
    csv_white.close();
    cout << "[get_reference] 세션 ID: " << session_id << " (dark/white 저장 완료)\n";
}

SpectroscopyData SpectroscopySensor::normalize_all_channels(){
    return normalize_all_channels(read_all_channels_with_bulb());
}

// 이미 측정된 raw 값을 전처리 (dark/white 레퍼런스 보정 -> 합이 1이 되도록 정규화)
SpectroscopyData SpectroscopySensor::normalize_all_channels(const SpectroscopyData& raw_data){
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

void SpectroscopySensor::collect_data_for_AI(string image_path){
    // 전처리 후(정규화) 데이터와 전처리 전(raw) 데이터를 서로 다른 파일에 저장
    ofstream csv_file;
    ofstream raw_csv_file;
    csv_file.open("./spectroscopy_datas/trash_spectroscopy_data.csv", ios::out | ios::app);
    raw_csv_file.open("./spectroscopy_datas/trash_spectroscopy_data_raw.csv", ios::out | ios::app);

    if(!csv_file.is_open() || !raw_csv_file.is_open()){
        cerr<<"[Spectroscopy] 데이터 파일을 열 수 없습니다 (./spectroscopy_datas 폴더 확인)\n";
        return;
    }

    // 두 파일 모두 같은 열 구성 (기존 열 뒤에 Memo, session_id 추가)
    const char* header = "Label,A,B,C,D,E,F,G,H,I,J,K,L,R,S,T,U,V,W,image_path,Memo,session_id\n";

    csv_file.seekp(0, ios::end);
    if(csv_file.tellp() == 0){
        csv_file<<header;
    }

    raw_csv_file.seekp(0, ios::end);
    if(raw_csv_file.tellp() == 0){
        raw_csv_file<<header;
    }

    int user_input =-1;
    while(true){

        for(const auto& pair:labels){
            cout<<"["<<pair.first<<"] "<< pair.second<<"\n";
        }

        cout<<"[Spectroscopy] 라벨 번호를 쓰세요\n";
        
        // 한 줄 전체를 읽어서 확인 (숫자가 아닌 입력 때문에 cin 이 에러 상태로 무한 반복되지 않도록)
        string input_line;
        if(!getline(cin, input_line)){
            cerr<<"[Spectroscopy] 입력이 종료되어 수집을 중단합니다\n";
            return;
        }

        if(!parse_int(trim(input_line), user_input) || labels.find(user_input) == labels.end()){
            cout<<"[Spectroscopy] 잘못된 번호입니다 다시 입력하세요\n";
            continue;
        }

        // 메모는 한 줄 전체를 입력받고, 그냥 엔터를 치면 빈 메모로 넘어감
        string note;
        cout<<"[Spectroscopy] 메모를 입력하세요 (없으면 엔터)\n";
        getline(cin, note);
        note = trim(note);

        // 한 번만 측정해서 raw(전처리 전)와 전처리 후 값을 둘 다 얻음
        SpectroscopyData raw_data = read_all_channels_with_bulb();
        SpectroscopyData cal_data = normalize_all_channels(raw_data);

        const string sid = session_id.empty() ? "unknown" : session_id;
        const string label = labels[user_input];

        // 전처리 후 데이터
        csv_file << label << ",";
        for(int i=0;i<18;i++){
            csv_file << cal_data.channel[i] << ",";
        }
        csv_file << csv_escape(image_path) << "," << csv_escape(note) << "," << sid << "\n";

        // 전처리 전(raw) 데이터
        raw_csv_file << label << ",";
        for(int i=0;i<18;i++){
            raw_csv_file << raw_data.channel[i] << ",";
        }
        raw_csv_file << csv_escape(image_path) << "," << csv_escape(note) << "," << sid << "\n";

        break;
    }
    csv_file.close();
    raw_csv_file.close();
}