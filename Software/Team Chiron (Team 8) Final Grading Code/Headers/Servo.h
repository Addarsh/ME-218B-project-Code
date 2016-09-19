#ifndef SERVO_H
#define SERVO_H

//Public functions
void WriteCameraServo(float angle);

//Private functions
static float ConvertAngle(float angle);
void WriteShooterServo(float angle);

#endif
