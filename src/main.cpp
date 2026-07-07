#include <iostream>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <wiringPi.h>

#include "config.h"
#include "common.h"
#include "event.h"
#include "network.h"

#include "sensor/camera.h"
#include "sensor/ultrasonic.h"
#include "sensor/spectroscopy.h"
#include "sensor/ledStrip.h"
#include "sensor/gps.h"

std::queue<PloggingData> uploadQueue;
std::mutex queueMutex;
GpsData globalGpsData;
std::mutex gpsMutex;

int main() {
    if (wiringPiSetup() == -1) {
        std::cerr << "[wiringPi] Error: set gpio\n";
        return -1;
    }

    SpectroscopySensor spectro;
    Camera cam;
    LedStrip led(LED_PIN);
    Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);
    GpsSensor gps(GPS_SERIAL_PORT);
    ServoManager servo(SERVO_TOP_LEFT, SERVO_TOP_RIGHT, SERVO_BOTTOM_LEFT, SERVO_BOTTOM_RIGHT);

    // init
    spectro.init();
    cam.init();
    led.init();
    ultrasonic.init();
    ultrasonic.set_base();
    gps.init();
    std::cout<<"\n[System] Initialized\n";

    // thread
    std::thread eventThread(event_processing_thread, std::ref(ultrasonic), std::ref(cam), std::ref(spectro), std::ref(led), std::ref(servo));
    std::thread httpThread(http_transmission_thread);

    eventThread.join();
    httpThread.join();

    return 0;
}
