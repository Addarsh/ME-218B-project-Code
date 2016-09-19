#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Port.h"
#include "termio.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "inc/hw_timer.h"
#include "PWM10Tiva.h"

#include "Servo.h"

#define SERVO_LOWER_DUTY 5
#define SERVO_UPPER_DUTY 10

#define CAMERASERVO 2
#define SHOOTERSERVO 3

void WriteCameraServo(float angle) {
    float Duty = ConvertAngle(angle);
    PWM_TIVA_SetDuty(Duty, CAMERASERVO);
}

void WriteShooterServo(float angle) {
    float Duty = ConvertAngle(angle);
    PWM_TIVA_SetDuty(Duty, SHOOTERSERVO);
}

//ConvertAngle
static float ConvertAngle(float angle) {
//Takes a float, returns a float.
    //Perform conversions between angle and corresponding duty cycle.
    float Duty = angle/((float) (SERVO_UPPER_DUTY-SERVO_LOWER_DUTY))+SERVO_LOWER_DUTY;
    return Duty; //Check that the conversions are correct, make sure that bounds are good
}

//End of Servo.c
