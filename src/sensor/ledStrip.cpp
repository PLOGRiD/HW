#include "ledStrip.h"
#include "config.h"
#include <wiringPi.h>
#include <softPwm.h>

LedStrip::LedStrip(int pin) : pin(pin) {}

void LedStrip::init() {
    pinMode(pin, OUTPUT);
    softPwmCreate(pin,0,40);
    // digitalWrite(pin, LOW);
}

void LedStrip::on() {
    digitalWrite(pin, HIGH);
    softPwmWrite(pin,(10/100.0)*40);
}

void LedStrip::off() {
    digitalWrite(pin, LOW);
}