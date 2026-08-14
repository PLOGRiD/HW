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
                // std::cout<<"[Raw GPS]"<<line<<"\n";
                // 줄바꿈을 만나면 파싱 시도
                if (parse_nmea(line, out_lat, out_lon, out_time)) {
                    data_updated = true; // 파싱 성공시 루프 탈출
                }
                else{
                    out_time = "2026-08-14T12:00:00";
                    data_updated=true; // temp for test
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
    std::cout<<"[GPS Thread]\n";
    // 1. 스레드가 시작될 때 GPS 센서 객체를 생성하고 초기화합니다.
    // 라즈베리파이4의 UART 포트 경로를 적어줍니다. 기본 하드웨어 시리얼은 보통 "/dev/ttyS0" 입니다.
    GpsSensor sensor(GPS_SERIAL_PORT); 
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