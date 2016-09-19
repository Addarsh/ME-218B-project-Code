/****************************************************************************
 Module
   Campaigning.c

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
#include "Campaigning.h"
#include "CaptureStation.h"
#include "FollowLine.h"
#include "MotorDrive.h"
#include "ActivateShooter.h"


//#define DEBUG_CAMPAIGN

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
static ES_Event DuringStateWaiting( ES_Event Event);
static ES_Event DuringStateFollowLine( ES_Event Event);
static ES_Event DuringStateCaptureStation( ES_Event Event);
static ES_Event DuringStateActivateShooter(ES_Event Event);


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, you could get
// away without it
static CampaignState_t CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMasterSM

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
bool InitCampaigningSM ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;  // save our priority
	
	//UpdatePWM(50, 50);
	

  ThisEvent.EventType = ES_ENTRY;
  // Start the Master State machine

  StartCampaigningSM( ThisEvent );

  return true;
}

/****************************************************************************
 Function
     PostMasterSM

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
bool PostCampaigningSM( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

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
ES_Event RunCampaigningSM (ES_Event CurrentEvent )
{
   bool MakeTransition = false;/* are we making a state transition? */
   CampaignState_t NextState = CurrentState;
   ES_Event EntryEventKind = { ES_ENTRY, 0 };// default to normal entry to new state
   ES_Event ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

    switch ( CurrentState )
   {
       case WaitingToStartCampaign:       // If current state is state one
         // Execute During function for state one. ES_ENTRY & ES_EXIT are
         // processed here allow the lowere level state machines to re-map
         // or consume the event
         CurrentEvent = DuringStateWaiting(CurrentEvent);
         //process any events
         if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
         {
            switch (CurrentEvent.EventType)
            {
               case START_FOLLOWING : //If event is event one
								 
                    #ifdef DEBUG_CAMPAIGN
                    printf("\rI recieved follow line command!\r\n");
                    #endif
                    //Execute action function for state one : event one 
                    NextState = FollowLine;//Decide what the next state will be
                    // for internal transitions, skip changing MakeTransition
                    MakeTransition = true; //mark that we are taking a transition
                    // if transitioning to a state with history change kind of entry
                    EntryEventKind.EventType = ES_ENTRY;
                    // optionally, consume or re-map this event for the upper
                    // level state machine
                    ReturnEvent.EventType = ES_NO_EVENT;
							 
                    //Post a START_FOLLOWING EVENT
                    ES_Event ThisEvent;
                    ThisEvent.EventType = START_FOLLOWING;
                    PostCampaigningSM(ThisEvent);
                  break;
                // repeat cases as required for relevant events
  
               case CAPTURE_STATION_NOW: //If event is event one
                      #ifdef DEBUG_CAMPAIGN
                      printf("\rI recieved capture station command!\r\n");
                      #endif
                      // Execute action function for state one : event one
                      NextState = CaptureStation;//Decide what the next state will be
                      // for internal transitions, skip changing MakeTransition
                      MakeTransition = true; //mark that we are taking a transition
                      // if transitioning to a state with history change kind of entry
                      EntryEventKind.EventType = ES_ENTRY;
                      // optionally, consume or re-map this event for the upper
                      // level state machine
                      ReturnEvent.EventType = ES_NO_EVENT;
                                 
                      //Post a CAPTURE_STATION_NOW
                      ThisEvent.EventType = CAPTURE_STATION_NOW;
                      PostCampaigningSM(ThisEvent);
                      break;
                // repeat cases as required for relevant events
              case STOP_NOW_CHANGE_TO_CAPTURE:
								 
                      //Stop motors now
                      UpdatePWM(50,50);
                      #ifdef DEBUG_CAMPAIGN
                      printf("\rI recieved capture station command STOPPING!\r\n");
                      #endif
                      // Execute action function for state one : event one
                      NextState = CaptureStation;//Decide what the next state will be
                      // for internal transitions, skip changing MakeTransition
                      MakeTransition = true; //mark that we are taking a transition
                      // if transitioning to a state with history change kind of entry
                      EntryEventKind.EventType = ES_ENTRY;
                      // optionally, consume or re-map this event for the upper
                      // level state machine
                      ReturnEvent.EventType = ES_NO_EVENT;
								
					  break;
							 
				case ACTIVATE_SHOOTER:
								 
                      //Stop motors now
                      UpdatePWM(50,50);
             
             
                      #ifdef DEBUG_CAMPAIGN
                      printf("\rI recieved activate shooting command in Campaigning SM!\r\n");
                      #endif
                      // Execute action function for state one : event one
                      NextState = ActivateShooter;//Decide what the next state will be
                      // for internal transitions, skip changing MakeTransition
                      MakeTransition = true; //mark that we are taking a transition
                      // if transitioning to a state with history change kind of entry
                      EntryEventKind.EventType = ES_ENTRY;
                      // optionally, consume or re-map this event for the upper
                      // level state machine
                      ReturnEvent.EventType = ES_NO_EVENT;
							 
                      //Post a ACTIVATE_SHOOTER to self
                      ThisEvent.EventType = ACTIVATE_SHOOTER;
                      PostCampaigningSM(ThisEvent);
                      break;
				}
            
         }
         break;
      // repeat state pattern as required for other states
	 
			case FollowLine:       // If current state is state one
					 // Execute During function for state one. ES_ENTRY & ES_EXIT are
					 // processed here allow the lowere level state machines to re-map
					 // or consume the event
					 CurrentEvent = DuringStateFollowLine(CurrentEvent);
						//printf("\rI am in follow line state!\r\n");
					 //process any events
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {
							switch (CurrentEvent.EventType)
							{
								 case CAPTURE_STATION_NOW : //If event is event one
										UpdatePWM(50,50);
										// Execute action function for state one : event one
										NextState = CaptureStation;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										EntryEventKind.EventType = ES_ENTRY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								 
										//Post a CAPTURE_STATION_NOW
										ES_Event ThisEvent;
										ThisEvent.EventType = CAPTURE_STATION_NOW;
										PostCampaigningSM(ThisEvent);
										break;
									// repeat cases as required for relevant events
								 
								  case STOP_NOW_CHANGE_TO_CAPTURE:
										
										UpdatePWM(50,50);
								 
										//printf("\rI recieved capture station command STOPPING!\r\n");
										// Execute action function for state one : event one
										NextState = CaptureStation;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										EntryEventKind.EventType = ES_ENTRY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								
										break;
									
								case ACTIVATE_SHOOTER:
								 
										//Stop motors now
										UpdatePWM(50,50);
									 
										#ifdef DEBUG_CAMPAIGN
										printf("\rI recieved activate shooting command in Campaigning SM!\r\n");
										#endif
										// Execute action function for state one : event one
										NextState = ActivateShooter;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										EntryEventKind.EventType = ES_ENTRY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								 
										//Post a ACTIVATE_SHOOTER to self
										ThisEvent.EventType = ACTIVATE_SHOOTER;
										PostCampaigningSM(ThisEvent);
										
								break;
							}
						}
						break;
						
				case CaptureStation:       // If current state is state one
					 // Execute During function for state one. ES_ENTRY & ES_EXIT are
					 // processed here allow the lowere level state machines to re-map
					 // or consume the event
					CurrentEvent = DuringStateCaptureStation(CurrentEvent);
					 //process any events
					#ifdef DEBUG_CAMPAIGN
					printf("\rI am in capture station state!\r\n");
					#endif
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {
							switch (CurrentEvent.EventType)
							{
								 case START_FOLLOWING : //If event is event one
										// Execute action function for state one : event one
										NextState = FollowLine;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										EntryEventKind.EventType = ES_ENTRY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								 
										//Post an event to motor following again
										//Post a START_FOLLOWING
										ES_Event ThisEvent;
										ThisEvent.EventType = START_FOLLOWING;
										PostCampaigningSM(ThisEvent);
										
										break;
									// repeat cases as required for relevant events
								 
								case ACTIVATE_SHOOTER:
								 
										//Stop motors now
										UpdatePWM(50,50);
									 
										#ifdef DEBUG_CAMPAIGN
										printf("\rI recieved activate shooting command in Campaigning SM!\r\n");
										#endif
										// Execute action function for state one : event one
										NextState = ActivateShooter;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										EntryEventKind.EventType = ES_ENTRY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								 
										//Post a ACTIVATE_SHOOTER to self
										ThisEvent.EventType = ACTIVATE_SHOOTER;
										PostCampaigningSM(ThisEvent);
										
										break;
							}
						}
						break;
				
				
				case ActivateShooter:       // If current state is state one
					 // Execute During function for state one. ES_ENTRY & ES_EXIT are
					 // processed here allow the lowere level state machines to re-map
					 // or consume the event
					CurrentEvent = DuringStateActivateShooter(CurrentEvent);
					 //process any events
					//printf("\rI am in Activte shooter state!\r\n");
					 if ( CurrentEvent.EventType != ES_NO_EVENT ) //If an event is active
					 {
							switch (CurrentEvent.EventType)
							{
								 case START_FOLLOWING : //If event is event one
										// Execute action function for state one : event one
										NextState = FollowLine;//Decide what the next state will be
										// for internal transitions, skip changing MakeTransition
										MakeTransition = true; //mark that we are taking a transition
										// if transitioning to a state with history change kind of entry
										EntryEventKind.EventType = ES_ENTRY;
										// optionally, consume or re-map this event for the upper
										// level state machine
										ReturnEvent.EventType = ES_NO_EVENT;
								 
										//Post an event to motor following again
										//Post a START_FOLLOWING
										ES_Event ThisEvent;
										ThisEvent.EventType = START_FOLLOWING;
										PostCampaigningSM(ThisEvent);
										
										break;
									// repeat cases as required for relevant events

													
							}
						}
						break;
						
						
				default:
						break;
	}
			
		
    //   If we are making a state transition
    if (MakeTransition == true)
    {
       //   Execute exit function for current state
       CurrentEvent.EventType = ES_EXIT;
       RunCampaigningSM(CurrentEvent);

       CurrentState = NextState; //Modify state variable

       // Execute entry function for new state
       // this defaults to ES_ENTRY
       RunCampaigningSM(EntryEventKind);
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
void StartCampaigningSM( ES_Event CurrentEvent )
{
  // if there is more than 1 state to the top level machine you will need 
  // to initialize the state variable
  CurrentState = WaitingToStartCampaign;
	
  // now we need to let the Run function init the lower level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  RunCampaigningSM(CurrentEvent);
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


static ES_Event DuringStateFollowLine( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
				#ifdef DEBUG_CAMPAIGN
				printf("\rEntered follow line state in campaign\r\n");
			#endif
        
        // after that start any lower level machines that run in this state
       StartFollowLine( Event );
			
				#ifdef DEBUG_CAMPAIGN
				printf("\rFinished initializing lower level SM\r\n");
				#endif
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			
				//Exit from lower state machine
				RunFollowLine(Event);
			
				//Stop motors
				UpdatePWM(50, 50);
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
			
				//Run loer  level SM
				ReturnEvent = RunFollowLine(Event);
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringStateCaptureStation( ES_Event Event)
{
    ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
       StartCaptureStation( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
			
				//Exit from lower state machine
				RunCaptureStation(Event);
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        // ReturnEvent = RunLowerLevelSM(Event);
      
        // repeat for any concurrent lower level machines
			
				//Run lower  level SM
				ReturnEvent = RunCaptureStation(Event);
      
        // do any activity that is repeated as long as we are in this state
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
}


static ES_Event DuringStateActivateShooter(ES_Event Event)
{
	
	 ES_Event ReturnEvent = Event; // assme no re-mapping or comsumption

    // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
    if ( (Event.EventType == ES_ENTRY) ||
         (Event.EventType == ES_ENTRY_HISTORY) )
    {
        // implement any entry actions required for this state machine
        
        // after that start any lower level machines that run in this state
        StartShooter( Event );
        // repeat the StartxxxSM() functions for concurrent state machines
        // on the lower level
    }
    else if ( Event.EventType == ES_EXIT )
    {
        // on exit, give the lower levels a chance to clean up first
        //RunLowerLevelSM(Event);
        // repeat for any concurrently running state machines
        // now do any local exit functionality
				RunShooter(Event);
      
    }else
    // do the 'during' function for this state
    {
        // run any lower level state machine
        ReturnEvent = RunShooter(Event);
      
        // repeat for any concurrent lower level machines
      
        // do any activity that is repeated as long as we are in this state
			
				
    }
    // return either Event, if you don't want to allow the lower level machine
    // to remap the current event, or ReturnEvent if you do want to allow it.
    return(ReturnEvent);
	
}
