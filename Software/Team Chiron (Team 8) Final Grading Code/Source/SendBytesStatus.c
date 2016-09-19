/****************************************************************************
 Module
   SendBytesStatus.c

 Revision
   2.0.1

 Description
   This is a template file for implementing state machines.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/17/16 14:58 a0s      team 8 tape sensor project
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
#include "ES_Configure.h"
#include "ES_Framework.h"
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

#define ALL_BITS (0Xff<<2)

//#define DEBUG_SENDBYTES

//Address Constants (For the sake of practice and learning)

//System Control Registers
#define SYSCTL_BASE 0x400FE000
#define RCGCGPIO 0x608 //Register 60, Page 340
#define RCGCSSI 0x61C //Register 64, Page 346

//SSI Registers
#define SSI_0_BASE 0x40008000
#define SSICC 0xFC8 //Register 11, Page 984
#define SSICR0 0x000 //Register 1, Page 969
#define SSICR1 0x004 //Register 2, Page 971
#define SSICPSR 0x010 //Register 5, Page 976
#define SSIIM 0x014 //Register 6, Page 977
#define SSIDR 0x008 //Register 3, Page 973
#define SSISR 0x00C //Register 4, Page 974
#define SSIRIS 0x018 //Register 7, Page 978
#define SSIMIS 0x01C //Register 8, Page 980

//GPIO Port Registers
#define GPIO_PORTA_APB_BASE 0x40004000
#define GPIO_PORTA_AHB_BASE 0x40058000
#define GPIO_AFSEL 0x420 //Register 10, Page 671
#define GPIO_PCTL 0x52C //Register 22, Page 688
#define GPIO_DIR 0x400 //Register 2, Page 663
#define GPIO_DEN 0x51C //Register 18, Page 682
#define GPIO_PUR 0x510 //Register 15, Page 678

//NVIC
#define NVIC_BASE 0xE000E000
#define EN0 0x100
#define EN1 0x104
#define EN2 0x108
#define EN3 0x10C

//End of Address Constants
#define TicksPerMS 40000


//#define DEBUG_SENDBYTES

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "SendBytesStatus.h"
#include "StatusPAC.h"
#include "strategy.h"
#include "Campaigning.h"
#include "CaptureStation.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE WAITING

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event DuringWaiting( ES_Event Event);
static ES_Event DuringSending( ES_Event Event);
static ES_Event DuringWaiting4EOT( ES_Event Event);
static ES_Event DuringWaiting4Timeout( ES_Event Event);
void WriteFIFO(uint8_t Data);
uint8_t ReadFIFO(void);
static bool isStatus = false;
static bool isPseudo = false;

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static SendBytesState_t CurrentState;
static uint8_t readdata[5];
static uint8_t getStatus[5] = {0xc0,0x00,0x00,0x00,0x00}; //Get status command array [FIXED]
static uint8_t getRequest[5] = {0x80,0x00,0x00,0x00,0x00}; //Request status command array
static uint8_t getQuery[5] = {0x70,0x00,0x00,0x00,0x00}; //Query status command array [FIXED]

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunSendBytesStatus

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
ES_Event RunSendBytesStatus( ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   SendBytesState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = CurrentEvent; // assume we are not consuming event

   switch ( CurrentState )
   {
       case WAITING_SENDBYTES :       // If current state is WAITING
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
                  // Execute action function for state one : event one
                  NextState = SENDING_SENDBYTES;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
							 
									//This is a status request
									isStatus = true;
							 
									PostStatusPAC(CurrentEvent);
                  break;
							 
							 case RETURN_REQUEST : //If event is RETURN_STATUS
                  // Execute action function for state one : event one
                  NextState = SENDING_SENDBYTES;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
							 
							 
									PostStatusPAC(CurrentEvent);
                  break;
							 
							 case RETURN_QUERY : //If event is RETURN_STATUS
                  // Execute action function for state one : event one
                  NextState = SENDING_SENDBYTES;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
							 
							 
									PostStatusPAC(CurrentEvent);
                  break;
                             case RETURN_PSEUDO_QUERY : //If event is RETURN_STATUS
                  // Execute action function for state one : event one
                  NextState = SENDING_SENDBYTES;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
							 
                                    isPseudo = true;
									PostStatusPAC(CurrentEvent);
                  break;    

            }
         }
         break;

	   case SENDING_SENDBYTES:       // If current state is SENDING
         // Execute During function for SENDING. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringSending(CurrentEvent);
					uint8_t freqCode;
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case RETURN_STATUS : //If event is GET_STATUS
								 
									#ifdef DEBUG_SENDBYTES
									printf("\rSending status\r\n");
									#endif
								 
									for (uint8_t i = 0; i < 5; i++)
									{
										WriteFIFO(getStatus[i]);
										#ifdef DEBUG_SENDBYTES
									  printf("%x\r\n",getStatus[i]);
										#endif
									}

									//Enable the NVIC interrupt for the SSI when starting to transmit (or receive?)
									HWREG(NVIC_BASE+EN0) |= BIT7HI; //Interrupt 7 for SSI0

                  // Execute action function for state one : event one
                  NextState = WAITING4EOT;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
                  break;
							 
							 case RETURN_REQUEST : //If event is GET_STATUS
								 
									freqCode = (uint8_t)CurrentEvent.EventParam;
									uint8_t myColor = getMyColor();
									uint8_t colorCode = ((myColor << 4) | (myColor << 5));
									getRequest[0] = 0x80 | freqCode | colorCode;
							 
									#ifdef DEBUG_SENDBYTES
									printf("\rCurrent Request\r\n");
									#endif
									for (uint8_t i = 0; i < 5; i++)
									{
										WriteFIFO(getRequest[i]);
										#ifdef DEBUG_SENDBYTES
										printf("%x\r\n",getRequest[i]);
										#endif
									}
									//Enable the NVIC interrupt for the SSI when starting to transmit (or receive?)
									HWREG(NVIC_BASE+EN0) |= BIT7HI; //Interrupt 7 for SSI0
                  // Execute action function for state one : event one
                  NextState = WAITING4EOT;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
                  break;
							 
							 case RETURN_QUERY : //If event is GET_STATUS
								 
									#ifdef DEBUG_SENDBYTES
									printf("\rSending\r\n");
									#endif
									for (uint8_t i = 0; i < 5; i++)
									{
										WriteFIFO(getQuery[i]);
										#ifdef DEBUG_SENDBYTES
									  printf("%x\r\n",getQuery[i]);
										#endif
									}
									//Enable the NVIC interrupt for the SSI when starting to transmit (or receive?)
									HWREG(NVIC_BASE+EN0) |= BIT7HI; //Interrupt 7 for SSI0
                  // Execute action function for state one : event one
                  NextState = WAITING4EOT;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
                  break;
                            
                                case RETURN_PSEUDO_QUERY : //If event is GET_STATUS
								 
									#ifdef DEBUG_SENDBYTES
									printf("\rSending\r\n");
									#endif
									for (uint8_t i = 0; i < 5; i++)
									{
										WriteFIFO(getQuery[i]);
										#ifdef DEBUG_SENDBYTES
									  printf("%x\r\n",getQuery[i]);
										#endif
									}
									//Enable the NVIC interrupt for the SSI when starting to transmit (or receive?)
									HWREG(NVIC_BASE+EN0) |= BIT7HI; //Interrupt 7 for SSI0
                  // Execute action function for state one : event one
                  NextState = WAITING4EOT;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
                  break;
                              
            }
         }
         break;
		 
	   case WAITING4EOT :       // If current state is WAITING4EOT
         // Execute During function for SENDING. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringWaiting4EOT(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case EOT_TIMEOUT : //If event is GET_STATUS
								#ifdef DEBUG_SENDBYTES
								if(!isStatus)
									{printf("\rReading result\r\n");
									}
									#endif
									for (uint8_t i = 0; i < 5; i++)
									{
										readdata[i] = ReadFIFO();
										#ifdef DEBUG_SENDBYTES
										if(!isStatus){printf("\r%x\r\n",readdata[i]);}
										#endif
									}
								
                  // Execute action function for state one : event one
                  NextState = WAITING4TIMEOUT;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY;
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
									ES_Timer_InitTimer(EOTTimer,2);
                  break;
            }
         }
         break;
		 
	   case WAITING4TIMEOUT :       // If current state is WAITING4TIMEOUT
         // Execute During function for SENDING. ES_ENTRY & ES_EXIT are
         // processed here allow the lower level state machines to re-map
         // or consume the event
         CurrentEvent = DuringWaiting4Timeout(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {   
							case ES_TIMEOUT:
								// Execute action function for state one : event one
								NextState =  WAITING_SENDBYTES;//Decide what the next state will be
								// for internal transitions, skip changing MakeTransition
								MakeTransition = true; //mark that we are taking a transition
								// if transitioning to a state with history change kind of entry
								EntryEventKind.EventType = ES_ENTRY;
								// optionally, consume or re-map this event for the upper
								// level state machine
								ES_Event CompletedEvent;
							
								
								CompletedEvent.EventType = PAC_COMM_COMPLETE;
								
								for (uint8_t i= 0; i < 5; i++)
								{
									CompletedEvent.EventArray[i] = readdata[i];
								}
								
								//If this is a status request, send back to only Strategy
								//Otherwise, send to only campaigning
								if(isStatus)
								{

									PostStrategySM(CompletedEvent);
									
									isStatus = false;
								}
                                else if(isPseudo)
								{
									isPseudo = false;
								}
								else
								{
									//return or query request returned to 
									//campagin

									PostCampaigningSM(CompletedEvent);
									
								}
			
								break;
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
       RunSendBytesStatus(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       //   Execute entry function for new state
       // this defaults to ES_ENTRY
       RunSendBytesStatus(EntryEventKind);
     }
     return(ReturnEvent);
}
/****************************************************************************
 Function
     StartSendBytesStatus

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
void StartSendBytesStatus ( ES_Event CurrentEvent )
{
   // to implement entry to a history state or directly to a substate
   // you can modify the initialization of the CurrentState variable
   // otherwise just start in the entry state every time the state machine
   // is started
   if ( ES_ENTRY_HISTORY != CurrentEvent.EventType )
   {
        CurrentState = WAITING_SENDBYTES;
   }
   // call the entry function (if any) for the ENTRY_STATE
   RunSendBytesStatus(CurrentEvent);
}

/****************************************************************************
 Function
     QuerySendBytesStatus

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
SendBytesState_t QuerySendBytesStatus ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event DuringWaiting( ES_Event Event)
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
			#ifdef DEBUG_SENDBYTES
				printf("\rEntered SendBytesPAC SM Waiting state\r\n");
			#endif
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			#ifdef DEBUG_SENDBYTES
				printf("\rExiting SendBytes PAC SM Waiting state\r\n");
			#endif
    }
	else
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

static ES_Event DuringSending( ES_Event Event)
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
			#ifdef DEBUG_SENDBYTES
				printf("\rEntered SendBytesPAC SM Sending state\r\n");
			#endif
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			#ifdef DEBUG_SENDBYTES
				printf("\rExiting SendBytesPAC SM Sending state\r\n");
			#endif

    }
	else
    // do the 'during' function for this state
    {
		
		}
		//ReturnEvent.EventType = ES_TIMEOUT; //To switch out
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);

        // repeat for any concurrent lower level machines

        // do any activity that is repeated as long as we are in this state
    
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringWaiting4EOT( ES_Event Event)
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
			#ifdef DEBUG_SENDBYTES
				printf("\rEntered SendBytesPAC SM WaitingForEOT state\r\n");
			#endif
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			#ifdef DEBUG_SENDBYTES
				printf("\rExiting SendBytesPAC SM WaitingForEOT state\r\n");
			#endif

    }
	else
    // do the 'during' function for this state
    {
		//ReturnEvent.EventType = ES_TIMEOUT; //To switch out
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);

        // repeat for any concurrent lower level machines

        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

static ES_Event DuringWaiting4Timeout( ES_Event Event)
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
			#ifdef DEBUG_SENDBYTES
				printf("\rEntering SendBytesPAC SM WaitingForTimeout state\r\n");
			#endif
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			#ifdef DEBUG_SENDBYTES
				printf("\rExiting SendBytesPAC SM WaitingForTimeout state\r\n");
			#endif

    }
	else
    // do the 'during' function for this state
    {
			//ReturnEvent.EventType = ES_TIMEOUT; //To switch out
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);

        // repeat for any concurrent lower level machines

        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}

void WriteFIFO(uint8_t Data)
{

	//Write to SSDIR register
    HWREG(SSI_0_BASE+SSIDR) = Data;
}

uint8_t ReadFIFO(void)
{

	 //Disable the NVIC interrupt for the SSI when receiving
    //HWREG(NVIC_BASE+EN0) &= BIT7LO; //Interrupt 7 for SSI0

	//Return reading from SSDIR
	return HWREG(SSI_0_BASE+SSIDR);
	
}

void ISRStatusPAC(void)
{
	//Clear interrupt
	HWREG(NVIC_BASE+EN0) &= BIT7LO;
	//PoRt an EOT event to Status PAC
	ES_Event ThisEvent;
	ThisEvent.EventType = EOT_TIMEOUT;
	
	PostStatusPAC(ThisEvent);
	
}

