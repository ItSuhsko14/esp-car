#include "motor_manager.h"
#include <Arduino.h>

void motorInit() {
    pinMode(MOTOR_LEFT_FWD, OUTPUT);
    pinMode(MOTOR_LEFT_BWD, OUTPUT);
    pinMode(MOTOR_RIGHT_FWD, OUTPUT);
    pinMode(MOTOR_RIGHT_BWD, OUTPUT);
    
    // Ensure motors are stopped at startup
    motorStop();
    
    Serial.println("Motor manager initialized");
    Serial.println("  Left:  GPIO 12 (FWD), GPIO 13 (BWD)");
    Serial.println("  Right: GPIO 14 (FWD), GPIO 15 (BWD)");
}

void motorForward() {
    digitalWrite(MOTOR_LEFT_FWD, LOW);
    digitalWrite(MOTOR_LEFT_BWD, HIGH);
    digitalWrite(MOTOR_RIGHT_FWD, LOW);
    digitalWrite(MOTOR_RIGHT_BWD, HIGH);
}

void motorBackward() {
    digitalWrite(MOTOR_LEFT_FWD, HIGH);
    digitalWrite(MOTOR_LEFT_BWD, LOW);
    digitalWrite(MOTOR_RIGHT_FWD, HIGH);
    digitalWrite(MOTOR_RIGHT_BWD, LOW);
}

void motorLeft() {
    // Tank turn left: right wheels forward, left wheels backward
    digitalWrite(MOTOR_LEFT_FWD, HIGH);
    digitalWrite(MOTOR_LEFT_BWD, LOW);
    digitalWrite(MOTOR_RIGHT_FWD, LOW);
    digitalWrite(MOTOR_RIGHT_BWD, HIGH);
}

void motorRight() {
    // Tank turn right: left wheels forward, right wheels backward
    digitalWrite(MOTOR_LEFT_FWD, LOW);
    digitalWrite(MOTOR_LEFT_BWD, HIGH);
    digitalWrite(MOTOR_RIGHT_FWD, HIGH);
    digitalWrite(MOTOR_RIGHT_BWD, LOW);
}

void motorStop() {
    digitalWrite(MOTOR_LEFT_FWD, LOW);
    digitalWrite(MOTOR_LEFT_BWD, LOW);
    digitalWrite(MOTOR_RIGHT_FWD, LOW);
    digitalWrite(MOTOR_RIGHT_BWD, LOW);
}


