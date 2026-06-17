#pragma once

#include "camera.h"
#include "sensor.h"

void eventProcessingThread(Ultrasonic& ultrasonic, Camera& cam, SpectroscopySensor& spec, LedStrip& led);
