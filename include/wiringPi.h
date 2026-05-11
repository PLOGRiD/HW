#pragma once

#define OUTPUT 1
#define INPUT 0
#define LOW 0
#define HIGH 1

inline int wiringPiSetup() { return 0; }
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline void delayMicroseconds(int) {}
inline unsigned int micros() { return 0; }
inline int digitalRead(int){ return 0; }
inline void delay(int) {}