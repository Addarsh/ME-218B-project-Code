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
#include "ADMulti.h"
#include "Camera.h"
#include "MotorDrive.h"

#include "GetOnLine.h"

#define CONTROL_LOOP_TIMEOUT 30 //ms
#define WINDUP_BOUND 50
#define PWM_SCALING 1.0
#define RIGHT_NOMINAL_DUTY 15 // 18
#define LEFT_NOMINAL_DUTY (RIGHT_NOMINAL_DUTY)*PWM_SCALING
#define NEUTRAL_DUTY 50
#define Kp 5
#define Ki 1
#define LINE_FOUND_COUNT_THRESHOLD 5


//Data private to this module: MyPriority, CurrentState, Error, AppendError, Command, Direction
static uint8_t MyPriority;
static GetOnLineState_t CurrentState;
static float Error = 0;
static int Direction = 1;
static float RightInput;
static float LeftInput;
static uint8_t LineFoundCount = 0;

void StartGetOnLine(ES_Event thisEvent) {
   // local variable to get debugger to display the value of CurrentEvent
   volatile ES_Event LocalEvent = thisEvent;
   CurrentState = WaitingToStart;  // always start in WaitingToStart
   // call the entry function (if any) for the ENTRY_STATE
   // there is no entry function for Expired
}

bool InitializeGetOnLine(uint8_t priority) {
    //Set MyPriority to priority.
    MyPriority = priority;
    //Set CurrentState to AlignWithBeacon
    CurrentState = WaitingToStart;
    //Generate timeout
    ES_Timer_InitTimer(DRIVE_TIMER_GET_ON_LINE, 1);
    //Return true
    return true;
}

bool PostGetOnLine(ES_Event thisEvent) {
    return ES_PostToService(MyPriority, thisEvent);
}

ES_Event RunGetOnLine(ES_Event thisEvent) {
    ES_Event ReturnVal;
    ReturnVal.EventType = ES_NO_EVENT;
    GetOnLineState_t NextState;
    NextState = CurrentState;
    //If there is a reversal command, change the directional multiplier
    if(thisEvent.EventType == ES_REVERSE_DRIVE) { //The corresponding event to rotate camera is in Camera.c
        Direction = -Direction;
    }
    Direction = 1; //HOTWIRE
    switch(CurrentState) {
        case WaitingToStart:
            if(thisEvent.EventType == ES_GET_ON_LINE) {
                NextState = AlignWithBeacon;
                ES_Timer_InitTimer(DRIVE_TIMER_GET_ON_LINE, 1);
            }
        break;
        case AlignWithBeacon:
            //Call AlignWithBeacon, AlignWithBeacon will send an event upon finding it
            //Check if the event prompts a switch to the FullSpeedForward state.
            if(thisEvent.EventType == ES_BEACON_FOUND) {
                NextState = FullSpeedForward;
                ES_Timer_InitTimer(DRIVE_TIMER_GET_ON_LINE, 1);
            }
        break;
        case FullSpeedForward:
            if(thisEvent.EventType == ES_TIMEOUT && thisEvent.EventParam == DRIVE_TIMER_GET_ON_LINE) {
                //See whether line has been found.
                bool LineFound = ReturnLineFound();
                if(LineFound && LineFoundCount > LINE_FOUND_COUNT_THRESHOLD) {
                    //Reset LineFoundCount
                    LineFoundCount = 0;
                    //Stop the motors
                    UpdatePWM(NEUTRAL_DUTY, NEUTRAL_DUTY);
                    //Set NextState to LineUp
                    NextState = LineUp;
                    //Generate Timeout to advance state.
                    ES_Timer_InitTimer(DRIVE_TIMER_GET_ON_LINE, 1);
                } else if(LineFound) {
                    LineFoundCount += 1;
                    ES_Timer_InitTimer(DRIVE_TIMER_GET_ON_LINE, CONTROL_LOOP_TIMEOUT);
                } else {
                    //Reset LineFoundCount (force it to get LINE_FOUND_COUNT_THRESHOLD LineFounds in a row)
                    LineFoundCount = 0;
                    //Drive both motors at full speed.
                    LeftInput = LEFT_NOMINAL_DUTY*Direction+NEUTRAL_DUTY;
                    RightInput = RIGHT_NOMINAL_DUTY*Direction+NEUTRAL_DUTY;
                    //Send PWM commands.
                    UpdatePWM(LeftInput, RightInput);
                    //Set a timer for the drive loop timeout.
                    ES_Timer_InitTimer(DRIVE_TIMER_GET_ON_LINE, CONTROL_LOOP_TIMEOUT);
                }
            }
        break;
        case LineUp:
            //Rotate until the error is on the interior side of the bot
            Error = ReturnLineCenter();
            if(Error > 0) {
                //Stop the motors
                UpdatePWM(NEUTRAL_DUTY, NEUTRAL_DUTY);
                //Change the state to FollowLine.
                ReturnVal.EventType = ES_LINE_FOUND;
            } else {
                //Rotate the motors CCW
                UpdatePWM(NEUTRAL_DUTY, NEUTRAL_DUTY+RIGHT_NOMINAL_DUTY);
                //Set the next state to be FullSpeedForward in case the rotation loses line
                NextState = FullSpeedForward;
                //Set a timeout
                ES_Timer_InitTimer(DRIVE_TIMER_GET_ON_LINE, CONTROL_LOOP_TIMEOUT);
            }            
        break;
    }
    if(thisEvent.EventType == ES_EXIT) {
        //Set NextState to be FollowWaiting.
        NextState = WaitingToStart;
        //Generate a timeout to advance the state machine.
        ES_Timer_InitTimer(DRIVE_TIMER_GET_ON_LINE, 1);
    }
    CurrentState = NextState;
    return ReturnVal;
}

//End of FollowLine.c
