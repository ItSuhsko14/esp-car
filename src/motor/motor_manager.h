#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

// GPIO pins for L298N driver
#define MOTOR_LEFT_FWD   12  // IN1 - Left wheels forward
#define MOTOR_LEFT_BWD   13  // IN2 - Left wheels backward
#define MOTOR_RIGHT_FWD  14  // IN3 - Right wheels forward
#define MOTOR_RIGHT_BWD  15  // IN4 - Right wheels backward

void motorInit();
void motorForward();
void motorBackward();
void motorLeft();
void motorRight();
void motorStop();

#endif


