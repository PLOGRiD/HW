#pragma once

// ultrasonic (5V)
#define TRIG_PIN 23
#define ECHO_PIN 24

// detect_event
#define DETECT_GAP 5.0
#define DETECT_COUNT 2

// camera
#define FRAME_WIDTH 1080
#define FRAME_HEIGHT 720

// led strip
#define LED_PIN 21

// gps
#define GPS_SERIAL_PORT "/dev/ttyAMA0"
#define GPS_BAUDRATE B9600

// servo
#define SERVO_TOP_LEFT 26
#define SERVO_TOP_RIGHT 27
#define SERVO_BOTTOM_LEFT 28
#define SERVO_BOTTOM_RIGHT 29

// event thread
#define DETECT_DELAY 50 // 초음파센서 측정 주기

