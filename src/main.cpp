#include <iostream>
#include <functional>
#include <thread>

#include "camera.h"
#include "sensor.h"
#include "config.h"
#include "common.h"
#include "event.h"
#include "network.h"

std::queue<PloggingData> uploadQueue;
std::mutex queueMutex;

int main() {
    if (wiringPiSetup() == -1) {
        std::cerr << "[wiringPi] Error: set gpio\n";
        return -1;
    }

    SpectroscopySensor spectro;
    Camera cam;
    LedStrip led(LED_PIN);
    Ultrasonic ultrasonic(TRIG_PIN, ECHO_PIN);

    // init
    spectro.init();
    cam.init();
    led.init();
    ultrasonic.init();
    ultrasonic.set_base();
    std::cout<<"\nfinish init\n";

    // thread
    std::thread eventThread(eventProcessingThread, std::ref(ultrasonic), std::ref(cam), std::ref(spectro), std::ref(led));
    std::thread httpThread(httpTransmissionThread);

    eventThread.join();
    httpThread.join();

    return 0;
}
