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

#include "Cannon.h"
#include "strategy.h"

#define CANNON_SERVO BIT4HI
#define SHOOT_DUTY 11
#define ARM_DUTY 8
#define RELOAD_DUTY 2.3

#define RELOAD_TIME 800
#define ARM_TIME 1000
#define SHOOT_TIME 800

//Variables private to the module: MyPriority, CurrentState, totalRotationTime
static uint8_t MyPriority;
static Cannon_State_t CurrentState;
static uint16_t totalRotationTime;

//InitCannon
bool InitCannon(uint8_t priority) {
    //Takes a priority number, returns true
    //Set MyPriority to priority.
    MyPriority = priority;
    //Set the current state to be CannonIdle.
    CurrentState = CannonIdle;
    //Return true.
    return true;
}

//PostCannon
bool PostCannon(ES_Event thisEvent) {
    //Takes an event, returns the result of posting this event
    return ES_PostToService(MyPriority, thisEvent);
}

ES_Event RunCannon(ES_Event thisEvent) {
    //Create a return value, and set to no event
    ES_Event ReturnVal;
    ReturnVal.EventType = ES_NO_EVENT;
    //Create NextState and set to CurrentState
    Cannon_State_t NextState = CurrentState;
    //Switch CurrentState
    switch(CurrentState) {
        //If CannonIdle
        case CannonIdle:
            //If it gets a shoot event
            if(thisEvent.EventType == ES_SHOOT) {
                //Create a timeout to move to the next state
                ES_Timer_InitTimer(CANNON_TIMER, 1);
                totalRotationTime = thisEvent.EventParam;
                //Set the next state to Reloading
                NextState = Reloading;
            }
            //Else if it gets a timeout
            else if(thisEvent.EventType == ES_TIMEOUT)
            {
                //Post shooting complete event
                ES_Event ThisEvent;
                ThisEvent.EventType = SHOOTING_COMPLETE;
                ThisEvent.EventParam = totalRotationTime;
                PostStrategySM(ThisEvent);
            }
        break;
        //If Reloading
        case Reloading:
            //If it gets a timeout
            if(thisEvent.EventType == ES_TIMEOUT) {
                //Set timer to for reload time
                ES_Timer_InitTimer(CANNON_TIMER, RELOAD_TIME);
                //Reload
                Reload();
                //Set the next state to be Arming
                NextState = Arming;
            }
        break;
        //If Arming
        case Arming:
            //If it gets a timeout
            if(thisEvent.EventType == ES_TIMEOUT) {
                //Set timer to for arm time
                ES_Timer_InitTimer(CANNON_TIMER, ARM_TIME);
                //Arm
                Arm();
                //Set next state to be shooting
                NextState = Shooting;
            }
        break;
        //If Shooting
        case Shooting:
            //If it gets a timeout
            if(thisEvent.EventType == ES_TIMEOUT) {
                //Set timer for shoot time
                ES_Timer_InitTimer(CANNON_TIMER, SHOOT_TIME);
                //Shoot
                Shoot();
                //Set next state to be idle
                NextState = CannonIdle;
            }
        break;
    }
    //Set current state to be next state
    CurrentState = NextState;
    //Return return value
    return ReturnVal;
}

//Shoot
static void Shoot(void) {
    //Takes nothing, returns nothing
    //Set the PWM duty for the servo to rotate to shoot position
    PWM_TIVA_SetDuty(SHOOT_DUTY, 2);
}

//Reload
static void Reload(void) {
    //Takes nothing, returns nothing
    //Set the PWM duty for the servo to rotate to reload position
    PWM_TIVA_SetDuty(RELOAD_DUTY, 2);
}

//Arm
static void Arm(void) {
    //Takes nothing, returns nothing
    //Set the PWM duty for the servo to rotate to arm position
    PWM_TIVA_SetDuty(ARM_DUTY, 2);
}
