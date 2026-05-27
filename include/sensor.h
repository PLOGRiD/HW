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
        std::string file_name;
        SpectroscopyData dark_ref;
        SpectroscopyData white_ref;

        float calcReflectance(float raw, float dark, float white);

    public:
        SpectroscopySensor();
        bool init();
        SpectroscopyData readAllChannelsWithBulb();
        SpectroscopyData readAllChannels();
        SpectroscopyData normalizeAllChannels();
        void collectDataForAI();
        void calibrateReferences();
};