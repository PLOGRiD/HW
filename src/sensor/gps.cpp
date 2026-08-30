#include "sensor/gps.h"
#include "config.h"
#include "common.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <vector>
#include <sstream>
#include <cstring>
#include <chrono>
#include <ctime>

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
    double latitude = 0.0;
    double longitude = 0.0;
    std::string timestamp = "";
    bool isValid = false;
    options.c_cflag |= CS8;              // 8 데이터 비트

    tcsetattr(serial_fd, TCSANOW, &options);
    std::cout<<"[GPS] Initialized\n";
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
            // GPS 토큰 시간(tokens[1])은 버리고 기기의 로컬 시간을 사용[cite: 1]
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm* now_tm = std::localtime(&now_c);

            char time_buf[30];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", now_tm);
            time = std::string(time_buf); // 예: "2026-08-14T17:30:15"

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
// 센서에서 데이터를 읽어와서 파싱을 시도하는 함수
void GpsSensor::update_gps(double& out_lat, double& out_lon, std::string& out_time) {
    if (serial_fd == -1) return;

    std::string line = "";

    while (true) {
        char c;
        int n = read(serial_fd, &c, 1);
        if (n > 0) {
            if (c == '\n') {
                // 1. $GPGGA 또는 $GNGGA 문장인지 확인
                if (line.find("$GPGGA") == 0 || line.find("$GNGGA") == 0) {
                    
                    // 2. 파싱 시도 (데이터가 텅 비었으면 무시됨)
                    parse_nmea(line, out_lat, out_lon, out_time);
                    
                    // 3. 데이터가 비어있든 채워져 있든, 문장을 한 번 검사했으면 루프 강제 탈출!
                    // (스레드가 무한정 멈춰있는 것을 방지)
                    return; 
                }
                line = ""; // 다른 종류의 NMEA 문장은 무시하고 다음 줄을 위해 초기화
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
    std::cout<<"[GPS Thread] Started\n";
    GpsSensor sensor(GPS_SERIAL_PORT); 
    sensor.init();

    while(true){
        // 매 루프마다 지역 변수를 0.0으로 싹 비워줍니다.
        double parsed_lat = 0.0;
        double parsed_lon = 0.0;
        std::string parsed_time = "";

        // 블로킹 없이 GNGGA 문장을 하나 읽고 바로 반환됨
        sensor.update_gps(parsed_lat, parsed_lon, parsed_time);

        {
            std::lock_guard<std::mutex> lock(gpsMutex);
            
            // 파싱된 위도/경도가 0.0이 아닐 때(위성을 잡아 정상 파싱되었을 때)만 유효값으로 처리
            if (parsed_lat != 0.0 && parsed_lon != 0.0) {
                globalGpsData.latitude = parsed_lat;
                globalGpsData.longitude = parsed_lon;
                globalGpsData.timestamp = parsed_time;
                globalGpsData.isValid = true;
            } else {
                // 아직 위성을 못 찾아서 0.0이 나왔다면 유효하지 않다고 표시
                globalGpsData.isValid = false; 
            }
        }

        // CPU 과점유 방지
        usleep(50000); // 50ms 대기
    }
}