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

    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    tcsetattr(serial_fd, TCSANOW, &options);
    std::cout<<"[GPS] Initialized\n";
}

bool GpsSensor::parse_nmea(const std::string& nmea_line, double& lat, double& lon, int64_t& time) {
    if (nmea_line.find("$GPGGA") == 0 || nmea_line.find("$GNGGA") == 0) {
        std::stringstream ss(nmea_line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 10 && !tokens[2].empty() && !tokens[4].empty()) {
            // 기기 로컬 시간을 epoch seconds로 저장
            time = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();

            double raw_lat = std::stod(tokens[2]);
            int lat_degrees = (int)(raw_lat / 100);
            double lat_minutes = raw_lat - (lat_degrees * 100);
            lat = lat_degrees + (lat_minutes / 60.0);
            if (tokens[3] == "S") lat = -lat;

            double raw_lon = std::stod(tokens[4]);
            int lon_degrees = (int)(raw_lon / 100);
            double lon_minutes = raw_lon - (lon_degrees * 100);
            lon = lon_degrees + (lon_minutes / 60.0);
            if (tokens[5] == "W") lon = -lon;

            return true;
        }
    }
    return false;
}

void GpsSensor::update_gps(double& out_lat, double& out_lon, int64_t& out_time) {
    if (serial_fd == -1) return;

    std::string line = "";

    while (true) {
        char c;
        int n = read(serial_fd, &c, 1);
        if (n > 0) {
            if (c == '\n') {
                if (line.find("$GPGGA") == 0 || line.find("$GNGGA") == 0) {
                    parse_nmea(line, out_lat, out_lon, out_time);
                    return;
                }
                line = "";
            } else if (c != '\r') {
                line += c;
            }
        } else {
            usleep(10000);
        }
    }
}

void gps_thread(){
    std::cout<<"[GPS Thread] Started\n";
    GpsSensor sensor(GPS_SERIAL_PORT);
    sensor.init();

    while(true){
        double parsed_lat = 0.0;
        double parsed_lon = 0.0;
        int64_t parsed_time = 0;   // std::string "" → int64_t 0

        sensor.update_gps(parsed_lat, parsed_lon, parsed_time);

        {
            std::lock_guard<std::mutex> lock(gpsMutex);

            if (parsed_lat != 0.0 && parsed_lon != 0.0) {
                globalGpsData.latitude = parsed_lat;
                globalGpsData.longitude = parsed_lon;
                globalGpsData.timestamp = parsed_time;

                std::cout<<"위도: "<<parsed_lat<<"\n경도: "<<parsed_lon<<"\n";
                globalGpsData.isValid = true;
            }
        }

        usleep(50000);
    }
}