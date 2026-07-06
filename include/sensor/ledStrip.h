#pragma once

class LedStrip {
private:
    int pin;
public:
    LedStrip(int pin_num);
    
    void init();
    void on();
    void off();
};