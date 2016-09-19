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

#include "MotorDrive.h"
#define ROTATE_CLOCKWISE_PWM 75
#define SERVO_PWM_DUTY 60

//RunMotorDrive
void UpdatePWM(float LeftPWM, float RightPWM) {
//Takes 2 PWM float values, returns nothing.

    PWM_TIVA_SetDuty(RightPWM, 1);
    PWM_TIVA_SetDuty(LeftPWM, 0);
	
	//printf("LEFT PWM:%f, LEFT PWM:%f\r\n",LeftPWM, RightPWM);
//End of UpdatePWM
}


void rotateClockwise(void)
{
		PWM_TIVA_SetDuty(25, 0);
    PWM_TIVA_SetDuty(ROTATE_CLOCKWISE_PWM, 1);
}


void rotateCounterClockwise(void)
{
		PWM_TIVA_SetDuty(25, 1);
    PWM_TIVA_SetDuty(ROTATE_CLOCKWISE_PWM, 0);
}


//Stop motors
void StopMotors(void)
{
	UpdatePWM(50,50);
}



void activateLauncherServo(void)
{
	//Activate PWM on servo
	//Launcher Servo is connected to PB5
	//PB5 is channel 3
	 PWM_TIVA_SetDuty(SERVO_PWM_DUTY, 3);
	
}