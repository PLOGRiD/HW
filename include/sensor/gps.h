#pragma once

#include <string>

class GpsSensor {
private:
    int serial_fd;
    std::string port_name;

    bool parse_nmea(const std::string& nmea_line, double& lat, double& lon, std::string& time);

public:
    GpsSensor(std::string port);
    ~GpsSensor();
    void init();
    void update_gps(double& out_lat, double& out_lon, std::string& out_time);
};

void gps_thread();