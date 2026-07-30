#pragma once

#include "sensor/camera.h"
#include "sensor/ultrasonic.h"
#include "sensor/spectroscopy.h"
#include "sensor/ledStrip.h"
#include "sensor/servo.h"

void event_processing_thread(Ultrasonic& ultrasonic_1, Ultrasonic& ultrasonic_2, Camera& cam, SpectroscopySensor& spec, LedStrip& led, ServoManager& servo);
void event_for_AI_Thread(Camera& cam, SpectroscopySensor& spec, LedStrip& led);
void event_for_test_Thread(Camera& cam, SpectroscopySensor& spec, LedStrip& led);