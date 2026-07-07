#pragma once

#include "sensor/camera.h"
#include "sensor/ultrasonic.h"
#include "sensor/spectroscopy.h"
#include "sensor/ledStrip.h"
#include "sensor/servo.h"

void event_processing_thread(Ultrasonic& ultrasonic, Camera& cam, SpectroscopySensor& spec, LedStrip& led, ServoManager& servo);
