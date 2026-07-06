#pragma once

#include "camera.h"
#include "sensor.h"

void event_processing_thread(Ultrasonic& ultrasonic, Camera& cam, SpectroscopySensor& spec, LedStrip& led);
