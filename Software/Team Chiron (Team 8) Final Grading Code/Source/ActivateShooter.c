/****************************************************************************
 Module
   ActivateShooter.c

 Revision
   2.0.1

 Description
   This is a template file for implementing state machines.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/07/13 21:00 jec      corrections to return variable (should have been
                         ReturnEvent, not CurrentEvent) and several EV_xxx
                         event names that were left over from the old version
 02/08/12 09:56 jec      revisions for the Events and Services Framework Gen2
 02/13/10 14:29 jec      revised Start and run to add new kind of entry function
                         to make implementing history entry cleaner
 02/13/10 12:29 jec      added NewEvent local variable to During function and
                         comments about using either it or Event as the return
 02/11/10 15:54 jec      more revised comments, removing last comment in during
                         function that belongs in the run function
 02/09/10 17:21 jec      updated comments about internal transitions on During funtion
 02/18/09 10:14 jec      removed redundant call to RunLowerlevelSM in EV_Entry
                         processing in During function
 02/20/07 21:37 jec      converted to use enumerated type for events & states
 02/13/05 19:38 jec      added support for self-transitions, reworked
                         to eliminate repeated transition code
 02/11/05 16:54 jec      converted to implment hierarchy explicitly
 02/25/03 10:32 jec      converted to take a passed event parameter
 02/18/99 10:19 jec      built template from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
/*#include "ES_Configure.h"
#include "ES_Framework.h"
#include "MotorDrive.h"*/
//#define DEBUG

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Port.h"
#include "termio.h"
#include "MotorDrive.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "inc/hw_timer.h"
#include "inc/hw_timer.h"
#include "PWM10Tiva.h"

#include "Cannon.h"


//#define DEBUG_CANNON

#define CANNON_SERVO BIT4HI
#define SHOOT_DUTY 11
#define ARM_DUTY 8
#define RELOAD_DUTY 2.3

#define RELOAD_TIME 800
#define ARM_TIME 500
#define SHOOT_TIME 800
#define PULL_BACK_TIME 200

#define BACK_PWM 10



/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ActivateShooter.h"
#include "init_all.h"
#include "strategy.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines
#define SHOOTING_TIME 2000 //2 seconds

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringStateWaiting( ES_Event Event);
static ES_Event DuringStateAligning( ES_Event Event);
static ES_Event DuringStateActivateMotor( ES_Event Event);
static ES_Event DuringStateGoBack( ES_Event Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static ShooterState_t CurrentState;
static uint16_t startRotationTime;
static uint16_t endRotationTime;
static uint16_t totalRotationTime;

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
ES_Event RunShooter( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   ShooterState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
       case WaitingToShoot :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringStateWaiting(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case ACTIVATE_SHOOTER : //If event is event one
									
                #ifdef DEBUG_CANNON
                printf("\rI got the Activate Shooter event!\r\n");
                #endif
                  NextState = AlignWithBeacons;
                  //Enable IR interrupts
                  enableIR();
			 
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
    case AlignWithBeacons :       // If current state is state one
       // Execute During function for state one. ES_ENTRY & ES_EXIT are
       // processed here allow the lower level state machines to re-map
       // or consume the event
       CurrentEvent = DuringStateAligning(CurrentEvent);
       //process any events
       if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
       {
          switch (CurrentEvent.EventType)
          {
             case ALIGN_COMPLETE : //If event is event one
                #ifdef DEBUG_CANNON
                printf("IR detected Align Complete!\r\n");
                #endif
                // Execute action function for state one : event one
                NextState = ActivateMotor;//Decide what the next state will be
                // for internal transitions, skip changing MakeTransition
                MakeTransition = true; //mark that we are taking a transition
                // if transitioning to a state with history change kind of entry
                EntryEventKind.EventType = ES_ENTRY;
                // optionally, consume or re-map this event for the upper
                // level state machine
             
           
             
                //Disable IR
                disableIR();
             
                endRotationTime = ES_Timer_GetTime();
             
                //Save returned rotation time
                totalRotationTime = endRotationTime - startRotationTime;
             
                ReturnEvent.EventType = ES_NO_EVENT;
                break;
              // repeat cases as required for relevant events
          }
       }
       break;
     
     case ActivateMotor :       // If current state is state one
       // Execute During function for state one. ES_ENTRY & ES_EXIT are
       // processed here allow the lower level state machines to re-map
       // or consume the event
       CurrentEvent = DuringStateActivateMotor(CurrentEvent);
       //process any events
       if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
       {
          switch (CurrentEvent.EventType)
          {
             case ES_TIMEOUT : //If event is event one
                if(CurrentEvent.EventParam == SHOOTING_COMPLETE)
                {

                  // Execute action function for state one : event one
                  NextState = WaitingToShoot;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
                  
                  // repeat cases as required for relevant events
                }
                break;
          }
       }
       break;
				 
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunShooter(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunShooter(EntryEventKind);
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
void StartShooter ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = WaitingToShoot;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunShooter(CurrentEvent);
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
ShooterState_t QueryShooter ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringStateWaiting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				#ifdef DEBUG_CANNON
				printf("\rEntered Waiting state of Activate Shooter state\r\n");
				#endif
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			
			
			#ifdef DEBUG_CANNON
			printf("\rExiting Waiting state of Activate Shooter state\r\n");
			#endif
      
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

static ES_Event DuringStateGoBack( ES_Event Event)
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
				#ifdef DEBUG_CANNON
				printf("\rEntered Waiting state of Activate Shooter state\r\n");
				#endif
        
        //Go back
        UpdatePWM(BACK_PWM,BACK_PWM);
        ES_Timer_InitTimer(PULL_BACK_TIMER,PULL_BACK_TIME);
        ES_Event MyEvent;
        MyEvent.EventType = GO_GO;
        PostCampaigningSM(MyEvent);
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			
			
			#ifdef DEBUG_CANNON
			printf("\rExiting Waiting state of Activate Shooter state\r\n");
			#endif
        
        //Stop motors
        StopMotors();
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
        
        //Go back
        UpdatePWM(BACK_PWM,BACK_PWM);
        ES_Event MyEvent;
        MyEvent.EventType = GO_GO;
        PostCampaigningSM(MyEvent);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringStateAligning( ES_Event Event)
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
				
				#ifdef DEBUG_CANNON
				printf("\rEntered Aligning state of Activate Shooter state\r\n");
				#endif
			
			
				ES_Timer_InitTimer(KEEP_ROTATING_CLOCKWISE,10);
			
			  //Send time as parameter Time
				startRotationTime = ES_Timer_GetTime();
			
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			
			//Stop motors
			StopMotors();
			
			#ifdef DEBUG_CANNON
			printf("\rExiting Aligning state of Activate Shooter state\r\n");
			#endif
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
			//Start rotating to align with beacon
			rotateCounterClockwise();
			ES_Timer_InitTimer(KEEP_ROTATING_CLOCKWISE,10);
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}



static ES_Event DuringStateActivateMotor( ES_Event Event)
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
			
				#ifdef DEBUG_CANNON
				printf("\rEntered Shooting state of Activate Shooter state\r\n");
				#endif
			
			
				//Activate arm
				//Post an event to shooter
				ES_Event ThisEvent;
				ThisEvent.EventType = ES_SHOOT;
				ThisEvent.EventParam = totalRotationTime;
				PostCannon(ThisEvent);

				//Initialize timer
				//ES_Timer_InitTimer(SHOOTER_MOTOR_TIMER, SHOOTING_TIME );
			
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			
			#ifdef DEBUG_CANNON
			printf("\rExiting Shooting state of Activate Shooter state\r\n");
			#endif
      
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

