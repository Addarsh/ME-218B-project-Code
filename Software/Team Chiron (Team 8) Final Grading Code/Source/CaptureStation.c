/****************************************************************************
 Module
   CaptureStation.c

 Revision
   2.0.1

 Description
   This is a template for the top level Hierarchical state machine

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
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
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "stdbool.h"
#include "CaptureStation.h"
#include "StatusPAC.h"
#include "strategy.h"
#include "Campaigning.h"


//#define CAPTURE_TEST
#define QUERY_DELAY_TIME 20
//#define DEBUG_CAPTURE_STATION

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/

static ES_Event DuringStateWaiting( ES_Event Event);
static ES_Event DuringStateWaitingForPAC( ES_Event Event);
static ES_Event DuringStateSendQueryPAC( ES_Event Event);
static ES_Event DuringStateWaitForQuery( ES_Event Event);
bool ProcessRequestResponse(uint8_t arr[]);
bool isResponseReady(uint8_t arr[]);
uint16_t read_frequency(void);
/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static Capture_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
//static uint8_t MyPriority;

static const uint8_t RESPONSE_READY = 0xaa;


/****************************************************************************
 Function
    RunMasterSM

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
ES_Event RunCaptureStation(ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   Capture_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   //ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error
	 ES_Event ReturnEvent = CurrentEvent;// assume no error

	 ES_Event ThisEvent;
    switch ( CurrentState )
   {
       case WAITING_CAPTURE :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringStateWaiting(CurrentEvent);
         //process any events
					
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case  CAPTURE_STATION_NOW	:
							 
									//Post Event to Request PAC with frequency
									ThisEvent = CurrentEvent;
									ThisEvent.EventType = RETURN_REQUEST;
									PostStatusPAC(ThisEvent);
							 
                  // Execute action function for state one : event one
                  NextState = WAIT_FOR_REQUEST_PAC;//Decide what the next state will be
                  // for internal transitions, skip changing MakeTransition
                  MakeTransition = true; //mark that we are taking a transition
                  // if transitioning to a state with history change kind of entry
                  EntryEventKind.EventType = ES_ENTRY_HISTORY;
							 
                  // optionally, consume or re-map this event for the upper
                  // level state machine
                  ReturnEvent.EventType = ES_NO_EVENT;
                  break;
                // repeat cases as required for relevant events
            }
         }
         break;
      // repeat state pattern as required for other states
				 
			case WAIT_FOR_REQUEST_PAC :       
				
				 CurrentEvent = DuringStateWaitingForPAC(CurrentEvent);
				 //process any events
				 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
				 {
					 bool result;
						switch (CurrentEvent.EventType)
						{
							 case  PAC_COMM_COMPLETE: //If event is event one
								
									//Result is good.Matches desired result.
									//Change state to send Query PAC
									
									#ifdef DEBUG_CAPTURE_STATION
									printf("Received from Request\r\n");
									#endif
									for(int i =0; i < 5;i++)
									{
										#ifdef DEBUG_CAPTURE_STATION
										printf("%d\r\n",CurrentEvent.EventArray[i]);
										#endif
									}
									
									// Execute action function for state one : event one
									NextState = SEND_QUERY_PAC;
									
									MakeTransition =true; //mark that we are taking a transition
									// if transitioning to a state with history change kind of entry
									
									
									//Start timer and wait for timeout before sending query
									ES_Timer_InitTimer(QUERY_TIMER, QUERY_DELAY_TIME);
									
							 
									//Enter with history
									EntryEventKind.EventType = ES_ENTRY_HISTORY;
							 
									//Consume event
									ReturnEvent.EventType = ES_NO_EVENT;
								//}
									break;
						}
				 }
				 break;
				 
		 case SEND_QUERY_PAC :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringStateSendQueryPAC(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType == ES_TIMEOUT && CurrentEvent.EventParam == QUERY_TIMER ) //If an event is active
         {
						//Post query
						//Post event to Query PAC	 
						ThisEvent.EventType = RETURN_QUERY;
						PostStatusPAC(ThisEvent);
						
						//Switch to new state
						NextState = WAIT_FOR_QUERY_RESPONSE;
						MakeTransition = true;
					 
							
						//Enter with history
						EntryEventKind.EventType = ES_ENTRY;
							
						//Consume event
						ReturnEvent.EventType = ES_NO_EVENT;
						break;
         }
         break;
				 
		 case WAIT_FOR_QUERY_RESPONSE :       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
				#ifdef DEBUG_CAPTURE_STATION
				printf("Inside WAIT_FOR_QUERY_RESPONSE\n\r");
				#endif
         CurrentEvent = DuringStateWaitForQuery(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
					 bool result;
            switch (CurrentEvent.EventType)
            {				
								case PAC_COMM_COMPLETE:
									
									// Check Query Response

									//Call function to check if reponse received is ready
									//to be processed
									result = isResponseReady(CurrentEvent.EventArray);
									if(result)
									{
										#ifdef DEBUG_CAPTURE_STATION
										printf("Query ready \r\n");
										#endif
									}
									else
									{
										#ifdef DEBUG_CAPTURE_STATION
										printf("Query not ready \r\n");
										#endif
									}
							 
									//If query received, send result to Strategy for processing
									//Go back to waiting state
									if(result)
									{
										#ifdef DEBUG_CAPTURE_STATION
										printf("Posting CAPTURE_STATION_RESULT \r\n");
										#endif
										
										ThisEvent = CurrentEvent;
										ThisEvent.EventType = CAPTURE_STATION_RESULT;
										PostStrategySM(ThisEvent);
										
										
										//Go back to waiting
										NextState = WAITING_CAPTURE;
										MakeTransition = true;
										
										//Enter with history
										EntryEventKind.EventType = ES_ENTRY;
										
										//Consume event
										ReturnEvent.EventType = ES_NO_EVENT;
									}
									else
									{
										//Switch to Send PAC Query state
					 
										NextState = SEND_QUERY_PAC;
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										EntryEventKind.EventType = ES_ENTRY_HISTORY;
										
										
										//sTART Timer before sending PAC
										//Start timer and wait for timeout before sending query
										ES_Timer_InitTimer(QUERY_TIMER, QUERY_DELAY_TIME);
										
							 
										ReturnEvent.EventType = ES_NO_EVENT;
									}
            }
         }
         break;
    }
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunCaptureStation(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunCaptureStation(EntryEventKind);
     }
   // in the absence of an error the top level state machine should
   // always return ES_NO_EVENT, which we initialized at the top of func
   return(ReturnEvent);
}
/****************************************************************************
 Function
     StartMasterSM

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
void StartCaptureStation ( ES_Event CurrentEvent )
{
  // if there is more than 1 state to the top level machine you will need 
  // to initialize the state variable
  CurrentState = WAITING_CAPTURE;
  // now we need to let the Run function init the lower level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  RunCaptureStation(CurrentEvent);
  return;
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
				
				//Insert code here to stop motors
			
			#ifdef DEBUG_CAPTURE_STATION
				printf("\rEntering Waiting State of CAPTURE_STATION\r\n");
			#endif
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				
				//No need to do anything in the Exit event
			
			#ifdef CAPTURE_TEST
				printf("\rEntering Exiting State of CAPTURE_STATION\r\n");
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



static ES_Event DuringStateWaitingForPAC( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
       //Do nothing
			#ifdef CAPTURE_TEST
				printf("\rEntering Waiting For PAC State of CAPTURE_STATION\r\n");
			#endif
			
				
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				
				//No need to do anything in the Exit event
			#ifdef CAPTURE_TEST
				printf("\rExiting Waiting For PAC State of CAPTURE_STATION\r\n");
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



static ES_Event DuringStateSendQueryPAC( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption
		ES_Event ThisEvent;
    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {

			#ifdef DEBUG_CAPTURE_STATION
			printf("\rEntering SendQuery of CAPTURE_STATION\r\n");
			#endif
			
			
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				
				//No need to do anything in the Exit event
			#ifdef DEBUG_CAPTURE_STATION
			printf("\rExiting SendQuery of CAPTURE_STATION\r\n");
			#endif
			
			ReturnEvent.EventType = ES_NO_EVENT;
			
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

static ES_Event DuringStateWaitForQuery( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
				//Do nothing
			#ifdef DEBUG_CAPTURE_STATION
			printf("\rEntering WaitForQuery of CAPTURE_STATION\r\n");
			#endif
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				
				//No need to do anything in the Exit event
			#ifdef DEBUG_CAPTURE_STATION
			printf("\rExiting WaitForQuery of CAPTURE_STATION\r\n");
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


/*Other private functions*/
bool isResponseReady(uint8_t arr[])
{
	
	uint8_t currResponse =  arr[2];
	if (currResponse == RESPONSE_READY)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ProcessRequestResponse(uint8_t arr[])
{
	uint8_t desiredArray[5] = {0x00,0xff,0x00,0x00,0x00};
	
	for(int i = 0;i < 5;i++)
	{
		if(desiredArray[i]!=arr[i])
		{
			return false;
		}
	}
	return true;
}

Capture_t QueryCaptureSM ( void )
{
	return CurrentState;
	
}
