/****************************************************************************
 Module
   StatusPAC.c

 Revision
   2.0.1

 Description
   This is a template for the top level Hierarchical state machine

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/17/16 14:15 a0s      team 8 tape sensor project
 02/08/12 01:39 jec      converted from MW_MasterMachine.c
 02/06/12 22:02 jec      converted to Gen 2 Events and Services Framework
 02/13/10 11:54 jec      converted During functions to return Event_t
                         so that they match the template
 02/21/07 17:04 jec      converted to pass Event_t to Start...()
 02/20/07 21:37 jec      converted to use enumerated type for events
 02/21/05 15:03 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "BITDEFS.H"
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "StatusPAC.h"

#define ALL_BITS (0Xff<<2)

#include "SendBytesStatus.h"
#include "StatusPAC.h"

//#define DEBUG_PAC

//Address Constants (For the sake of practice and learning)



/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
static ES_Event DuringWaiting( ES_Event Event);
static ES_Event DuringSendingStatus( ES_Event Event);
uint8_t querycolor(void);
void InitPins(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static StatusPACState_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
//To decide which team the player is playing for
static uint8_t color; //Manual color selection

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitStatusPAC

 Parameters
     uint8_t : the priorty of this service

 Returns
     boolean, False if error in initialization, True otherwise

 Description
     Saves away the priority,  and starts
     the top level state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:06
****************************************************************************/
bool InitStatusPAC ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;  // save our priority

  ThisEvent.EventType = ES_ENTRY;
  // Start the Master State machine
  InitPins();
  StartStatusPAC( ThisEvent );
	
	

  return true;
}

/****************************************************************************
 Function
     PostStatusPAC

 Parameters
     ES_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the post operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostStatusPAC( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunStatusPAC

 Parameters
   ES_Event: the event to process

 Returns
   ES_Event: an event to return

 Description
   the run function for the top level state machine 
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 02/06/12, 22:09
****************************************************************************/
ES_Event RunStatusPAC( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   StatusPACState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

    switch ( CurrentState )
   {
       case WAITING_PAC :       // If current state is WAITING
         // Execute During function for WAITING. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringWaiting(CurrentEvent);
         //process any events
			 
					
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case RETURN_STATUS : //If event is RETURN_STATUS
									
                  // Execute action function for WAITING : RETURN_STATUS
                  NextState = SENDING;//Switch to next state
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
									//Post same event to service
									PostStatusPAC(CurrentEvent);
                  break;
                // repeat cases as required for relevant events
							 
							 case RETURN_REQUEST : //If event is RETURN_STATUS
									
                  // Execute action function for WAITING : RETURN_STATUS
                  NextState = SENDING;//Switch to next state
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
									//Post same event to service
									PostStatusPAC(CurrentEvent);
                  break;
                // repeat cases as required for relevant events
							 
							 case RETURN_QUERY : //If event is RETURN_STATUS
									
                  // Execute action function for WAITING : RETURN_STATUS
                  NextState = SENDING;//Switch to next state
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
									//Post same event to service
									PostStatusPAC(CurrentEvent);
                  break;
                // repeat cases as required for relevant events
            }
         }
         break;
		 
		 case SENDING :       // If current state is SENDING_STATUS
         // Execute During function for WAITING. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringSendingStatus(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
                // repeat cases as required for relevant events
               case ES_TIMEOUT: //If event is GET_STATUS
                  // Execute action function for WAITING : RETURN_STATUS
                  NextState = WAITING_PAC;//Switch to next state
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
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunStatusPAC(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunStatusPAC(EntryEventKind);
     }
   // in the absence of an error the top level state machine should
   // always return ES_NO_EVENT, which we initialized at the top of func
   return(ReturnEvent);
}
/****************************************************************************
 Function
     StartStatusPAC
	 
 Parameters
     ES_Event CurrentEvent

 Returns
     nothing

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:15
****************************************************************************/
void StartStatusPAC ( ES_Event CurrentEvent )
{
  // if there is more than 1 state to the top level machine you will need 
  // to initialize the state variable
  CurrentState = WAITING_PAC;
  // now we need to let the Run function init the lower level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  RunStatusPAC(CurrentEvent);
  return;
}


/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringWaiting( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assume no re-mapping or consumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        //StartLowerLevelSM( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
			#ifdef DEBUG_PAC				
					printf("\rEntered Status PAC SM Waiting state\r\n");
			#endif
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			#ifdef DEBUG_PAC
			printf("\rExiting Status PAC SM Waiting state\r\n");
			#endif
      
    }
	else
    // do the 'during' function for this state
    {
		 //remove post debugging
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringSendingStatus( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
			#ifdef DEBUG_PAC
        printf("\rEntered Status PAC SM Sending state\r\n");
			#endif
        // after that start any lower level machines that run in this state
        StartSendBytesStatus(Event);
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
				
				//Return a new state
				
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        RunSendBytesStatus(Event);
			
			#ifdef DEBUG_PAC
				printf("\rExiting Status PAC SM Sending state\r\n");
			#endif
        // repeat for any concurrently running state machines
        // now do any local exit functionality
      
    }
	else
    // do the 'during' function for this state
    {
		//puts("\r Sending Status \n\n"); //remove post debugging
        // run any lower level state machine
        ReturnEvent = RunSendBytesStatus(Event);      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


void InitPins(void) //Requires to be appropriately configured later
{
	
	// Pin PE0 being used to determine color
	// If PE0 = LO (BLUE), PE0 = HI (RED)
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4; 
    HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (GPIO_PIN_0); //Initialize Port E Bit 0 to be digital
    
    //Manual delay to ensure clock is ready
    while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R4) != SYSCTL_PRGPIO_R4);
    HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) &= ~(GPIO_PIN_0); //Initialize Port E Bit 0 to be input
}

uint8_t querycolor(void)
{
	color = HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA + ALL_BITS)); //CHANGE THIS ACCORDINGLY WITH RESPECT TO PIN INTIALIZED FOR THIS OP
	return color;
}
