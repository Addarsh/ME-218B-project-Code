#define LOWPASS
//#define INVERSION_LOGIC
#define ADAPTIVE_SPEED


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
#include "ultrasonic.h"

#include "FollowLine.h"
#define CONTROL_LOOP_TIMEOUT 40 //40 ms
#define WINDUP_BOUND 2
#define ACCUMULATED_WINDUP_BOUND 30
#define PWM_SCALING 1.0
#define RIGHT_NOMINAL_DUTY 40 // 40
#define LEFT_NOMINAL_DUTY (RIGHT_NOMINAL_DUTY)*PWM_SCALING
#define NEUTRAL_DUTY 50
#define Kp 55 // 55
#define Ki 6
#define Kd 10
#define Ks 0.03
#define NUM_PAST_CENTER 2

//PWM DEADBANDS L: 43, 57, R: 43, 57
#define LEFT_LOWER_DEADBAND 48
#define LEFT_UPPER_DEADBAND 52
#define RIGHT_LOWER_DEADBAND 48
#define RIGHT_UPPER_DEADBAND 52

//Finding the line
#define LINE_LOST_COUNTER_THRESHOLD 15 //How many cycles it loses the line before going to find line
#define DISTANCE_THRESHOLD 0.08
#define ROTATION_TIME 1600 //ms for rotation
#define NUM_PAST_PROXIMITY 10
#define LOWER_DISTANCE_THRESHOLD 0.02
#define REVERSE_TIME 1000


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringStateWaiting( ES_Event Event);
static ES_Event DuringStateFollowing( ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well

//Data private to this module: MyPriority, CurrentState, Error,
//AppendError, Command, Direction, RightInput, LeftInput,
//PastLine, PastLineAverage, InPWMDeadband, PreviousError,
//Derivative, SpeedReduction, LineLostCounter, LineLostRecovery,
//LineLostRecovery, PastProximity, AverageProximity, distance
static FollowLine_t CurrentState;
static float Error = 0;
static float AccumulatedError = 0;
static float AppendError = 0;
static float Command;
static int Direction = 1;
static float RightInput;
static float LeftInput;
static float PastLine[NUM_PAST_CENTER];
static float PastLineAverage;
static bool InPWMDeadband;
static float PreviousError = 0;
static float Derivative;
static float SpeedReduction = 0;
static uint32_t LineLostCounter = 0;
static bool LineLostRecovery = true;
static float PastProximity[NUM_PAST_PROXIMITY];
static float AverageProximity = 0;
static float distance = 0.12;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunTemplateSM

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event RunFollowLine(ES_Event CurrentEvent) {
    bool MakeTransition = false;/* are we making a state transition? */
    FollowLine_t NextState = CurrentState;
    ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
    ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event
    switch (CurrentState) {
        case FollowWaiting:// If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        CurrentEvent = DuringStateWaiting(CurrentEvent);
        //process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) {//If an event is active
            switch (CurrentEvent.EventType) {
                case START_FOLLOWING: //If event is event one
                //printf("\rStarted following\r\n");
                // Execute action function for state one : event one
                NextState = Following;//Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; //mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
                // repeat cases as required for relevant events
            }
        }
        break;
        // repeat state pattern as required for other states
				
    case Following:// If current state is state one
        // Execute During function for state one. ES_ENTRY & ES_EXIT are
        // processed here allow the lower level state machines to re-map
        // or consume the event
        CurrentEvent = DuringStateFollowing(CurrentEvent);
        //process any events
        if (CurrentEvent.EventType != ES_NO_EVENT) { //If an event is active
            switch (CurrentEvent.EventType) {
                case ROTATE180:
                    NextState = Reversing;
                    MakeTransition = true;
                    EntryEventKind.EventType = ES_ENTRY;
                    ReturnEvent.EventType = ES_NO_EVENT;
                break;
            }	
        }
        break;
        case Reversing:
            StopMotors();
            CurrentEvent = DuringStateReversing(CurrentEvent);
            if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == DRIVE_TIMER) {
                    StopMotors();
                    NextState = Rotating;
                    MakeTransition = true;
                    EntryEventKind.EventType = ES_ENTRY;
                    ReturnEvent.EventType = ES_NO_EVENT;
            }
        break;
     case Rotating:
        StopMotors();
        CurrentEvent = DuringStateRotating(CurrentEvent);
        if(CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == DRIVE_TIMER) {
                StopMotors();
                NextState = Following;
                MakeTransition = true;
                EntryEventKind.EventType = ES_ENTRY;
                ReturnEvent.EventType = ES_NO_EVENT;
        }
        break;
      // repeat state pattern as required for other states
    }
    //   If we are making a state transition
    if (MakeTransition == true) {
       //Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunFollowLine(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //Execute entry function for new state
       //This defaults to ES_ENTRY
       RunFollowLine(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartTemplateSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartFollowLine(ES_Event CurrentEvent) {
    // to implement entry to a history state or directly to a substate
    // you can modify the initialization of the CurrentState variable
    // otherwise just start in the entry state every time the state machine
    // is started
    if (ES_ENTRY_HISTORY != CurrentEvent.EventType) {
        CurrentState = FollowWaiting;
    }
    ES_Timer_InitTimer(DRIVE_TIMER, 1);
    // call the entry function (if any) for the ENTRY_STATE
    RunFollowLine(CurrentEvent);
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
FollowLine_t QueryFollowLine (void) {
    return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringStateWaiting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assume no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringStateFollowing( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
			ES_Timer_InitTimer(DRIVE_TIMER, CONTROL_LOOP_TIMEOUT);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
			
      
        // repeat for any concurrent lower level machines
			
			
				//Get the current line position from Camera.
                Error = ReturnLineCenter();
                //Inversion logic says to make error opposite of last lost line
                #ifdef INVERSION_LOGIC
                if(!ReturnLineFound) {
                    Error = -Error;
                }
                #endif
                //Lowpass applies a filter to previous line centers
                #ifdef LOWPASS
                //Only add the line center to the average if it was found
                if(ReturnLineFound()) {
                    //Set LineLostRecovery to false, since line is found
                    LineLostRecovery = false;
                    //Set the past line average to 0, and calculate new running average
                    PastLineAverage = 0;
                    LineLostCounter = 0; //Line is found, so reset the line lost counter
                    for (int i = 0; i < NUM_PAST_CENTER-1; i+= 1) {
                        PastLine[i+1] = PastLine[i];
                    }
                    PastLine[0] = Error;
                    for (int i = 0; i < NUM_PAST_CENTER; i+= 1) {
                        PastLineAverage += PastLine[i];
                    }
                    PastLineAverage /= NUM_PAST_CENTER;
                } else {
                    LineLostCounter += 1;
                }
                //Set the error to the previous average
                Error = PastLineAverage;
                #endif
                
                //Implement anti-windup logic and I-control.
                //If the accumulated error is greater than some bound, and error is positive,
                //don't add to the accumulated error
                if(AccumulatedError > WINDUP_BOUND && Error > 0) {
                    AppendError = 0;
                //If the accumulated error is less than than negative of some bound, and error is negative,
                //don't add to the accumulated error
                } else if(AccumulatedError < -WINDUP_BOUND && Error < 0) {
                    AppendError = 0;
                //Otherwise, add to the accumulated error
                } else {
                    AppendError = (float) Error;
                }
                AccumulatedError += ((float) CONTROL_LOOP_TIMEOUT/1000)*AppendError;
                
                //Implement D-control
                Derivative = (Error - PreviousError)/((float) CONTROL_LOOP_TIMEOUT/1000);
                //Set the previous error to the current error
                PreviousError = Error;
                
                //Make the robot follow the line by slowing down one of the motors by a certain amount
                //and leaving the other motor at the nominal PWM setting
                Command = Kp*Error+Ki*AccumulatedError+Kd*Derivative;

                //Implement adaptive speed, which slows the car down based on amount of turning
                #ifdef ADAPTIVE_SPEED
                SpeedReduction = Ks*Command;
                if(SpeedReduction > RIGHT_NOMINAL_DUTY) {
                    SpeedReduction = RIGHT_NOMINAL_DUTY;
                }
                #else
                SpeedReduction = 0;
                #endif
                //If the line has been lost for more than LINE_LOST_COUNTER_THRESHOLD cycles,
                //make the car go straight
                if(LineLostCounter > LINE_LOST_COUNTER_THRESHOLD) {
                    LineLostRecovery = true;
                }
                //Based on the calculated command, slow down one of the wheels, saturate command if too large
                if(Command >= 0) {
                    if((RIGHT_NOMINAL_DUTY+NEUTRAL_DUTY - Command) < RIGHT_UPPER_DEADBAND) {
                        Command = RIGHT_UPPER_DEADBAND - (RIGHT_NOMINAL_DUTY+NEUTRAL_DUTY - Command);
                        InPWMDeadband = true;
                    } else {
                        InPWMDeadband = false;
                    }
                    LeftInput = LEFT_NOMINAL_DUTY*Direction+NEUTRAL_DUTY-SpeedReduction;
                    if(InPWMDeadband) {
                        RightInput = RIGHT_LOWER_DEADBAND - Command;
                    } else {
                        RightInput = (RIGHT_NOMINAL_DUTY-Command)*Direction+NEUTRAL_DUTY-SpeedReduction;
                    }
                    if(LeftInput > 99) {
                        LeftInput = 99;
                    } else if(LeftInput < 1) {
                        LeftInput = 1;
                    }
                    if(RightInput > 99) {
                        RightInput = 99;
                    } else if(RightInput < 1) {
                        RightInput = 1;
                    }
                    UpdatePWM(LeftInput, RightInput); //May be wrong (opposite)
                } else if(Command < 0) {
                    if((LEFT_NOMINAL_DUTY + PWM_SCALING*Command+NEUTRAL_DUTY) < LEFT_UPPER_DEADBAND) {
                        Command = LEFT_UPPER_DEADBAND - (LEFT_NOMINAL_DUTY+NEUTRAL_DUTY + PWM_SCALING*Command);
                        InPWMDeadband = true;
                    } else {
                        InPWMDeadband = false;
                    }
                    RightInput = RIGHT_NOMINAL_DUTY*Direction+NEUTRAL_DUTY-SpeedReduction;
                    if(InPWMDeadband) {
                        LeftInput = LEFT_LOWER_DEADBAND - Command;
                    } else {
                        LeftInput = (LEFT_NOMINAL_DUTY+PWM_SCALING*Command)*Direction+NEUTRAL_DUTY-SpeedReduction;
                    }
                    if(LeftInput > 99) {
                        LeftInput = 99;
                    } else if(LeftInput < 1) {
                        LeftInput = 1;
                    }
                    if(RightInput > 99) {
                        RightInput = 99;
                    } else if(RightInput < 1) {
                        RightInput = 1;
                    }
                    UpdatePWM(LeftInput, RightInput); //May be wrong (opposite)
                }
                
                //If the line is lost, ignore everything before and go straight
                if(LineLostRecovery) {
                    UpdatePWM(50+20*PWM_SCALING, 50+20);
                }
                //Poll the ultrasonic sensor reading
                float pseudoDistance = querydistance();
                //Add some logic to get rid of results that don't make sense from the ultrasonic
                if(pseudoDistance > LOWER_DISTANCE_THRESHOLD && !(pseudoDistance > 0.069 &&
                    pseudoDistance < 0.079)) {
                    distance = pseudoDistance;
                } else {
                    for (int i = 0; i < NUM_PAST_PROXIMITY-1; i+= 1) {
                        PastProximity[i] = 0.12;
                    }
                }
                //Implement a low pass filter for the ultrasonic results
                AverageProximity = 0;
                for (int i = 0; i < NUM_PAST_PROXIMITY-1; i+= 1) {
                    PastProximity[i] = PastProximity[i+1];
                }
                PastProximity[NUM_PAST_PROXIMITY-1] = distance;
                for (int i = 0; i < NUM_PAST_PROXIMITY; i+= 1) {
                    AverageProximity += PastProximity[i];
                }
                AverageProximity /= NUM_PAST_PROXIMITY;
                //If the distance is within a threshold (too close), rotate 180
                if (AverageProximity > LOWER_DISTANCE_THRESHOLD && AverageProximity < DISTANCE_THRESHOLD &&
                    LineLostRecovery) {
                    StopMotors();
                    ReturnEvent.EventType = ROTATE180;
                    //After rotating 180 degrees flush the low pass filter
                    for(int i = 0; i < NUM_PAST_PROXIMITY; i += 1) {
                        PastProximity[i] = DISTANCE_THRESHOLD + 0.05;
                    }
                } else {
                    //Set a timer for the drive loop timeout.
                    ES_Timer_InitTimer(DRIVE_TIMER, CONTROL_LOOP_TIMEOUT);
                }
      
        // do any activity that is repeated as long as we are in this state
    }
        // return either Event, if you don't want to allow the lower level machine
        // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringStateRotating(ES_Event Event) {
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ((Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY)) {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
        rotateCounterClockwise();
        //LineLostCounter = 0;
        ES_Timer_InitTimer(DRIVE_TIMER, ROTATION_TIME);
    } else if (Event.EventType == ES_EXIT) {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    } else { // do the 'during' function for this state
        // return either Event, if you don't want to allow the lower level machine
        // to remap the current event, or ReturnEvent if you do want to allow it.
    }
    return(ReturnEvent);
}
static ES_Event DuringStateReversing(ES_Event Event) {
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) ) {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
        
        UpdatePWM(30, 30);
        ES_Timer_InitTimer(DRIVE_TIMER, REVERSE_TIME);
    } else if ( Event.EventType == ES_EXIT ) {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    } else {// do the 'during' function for this state
        // return either Event, if you don't want to allow the lower level machine
        // to remap the current event, or ReturnEvent if you do want to allow it.
    }
    return(ReturnEvent);
}
