#pragma once

#include <string>
#include <map>
#include <stdint.h>
#include "SparkFun_AS7265X.h"

struct SpectroscopyData{
    float A, B, C, D, E, F;
    float G, H, I, J, K, L;
    float R, S, T, U, V, W; 
};

class SpectroscopySensor{
    private:
        AS7265X as7265x;
        std::map<int, std::string> labels;
        std::string filename;

    public:
    SpectroscopySensor();
    bool init();
    SpectroscopyData readAllChannels();
    void collect_data_for_ai();
};