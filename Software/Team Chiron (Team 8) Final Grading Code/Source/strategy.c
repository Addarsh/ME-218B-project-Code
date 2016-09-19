/****************************************************************************
 Module
   strategySM.c

 Revision
   1.0.1

 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "strategy.h"
#include "StatusPAC.h"
#include "ES_Events.h"
#include "CaptureStation.h"
#include "inc/hw_nvic.h"

#include "init_all.h"
#include "Campaigning.h"
#include "MotorDrive.h"
#include "Math.h"
#include "Direction.h"




/*----------------------------- Module Defines ----------------------------*/
#define MIN_PERIOD 500
#define MAX_PERIOD 1333
#define NOT_ACKNOWLEDGED 0
#define ACKNOWLEDGED 64
#define BLOCKED 192
#define BUSY 128
#define SEATTLE_CODE 2
#define MIAMI_CODE 7
#define RED_ATTACK_STATUS_BYTE 1
#define BLUE_ATTACK_STATUS BYTE 2
#define ROTATING_TIME 3000 ///3second timeout


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static uint8_t getPollingStationCode(void);
static uint8_t processQueryResponse(uint8_t dataArray[]);
static uint8_t processStatusResponse(uint8_t dataArray[]);
static void clearLOC(void);
static uint8_t getACK(void);
static uint8_t getRBN(void);
static uint8_t getLOC(void);

static uint8_t SS3;
static bool startedGame =false;
static bool ShootingInProgress = false;
static bool ShootingComplete = false;
static float errorInFrequency = 25;
static uint8_t numShootingAttempts = 0;
static uint8_t numBallsLeft = 0;
static bool firstTimeFrequencyFound = false;
static bool LocationUpdated = false;
static int BigTimerCount = 0;

static uint16_t BIG_TIMER_COUNT =46050;


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static MyState_t CurrentState;
static uint8_t ResponseReady;
static uint8_t RequestStatus;
static uint8_t ACK;
static uint8_t RBN;
static uint8_t LOC;
static int RequestCount = 0;
static uint8_t NACK_COUNT = 0;
static uint8_t RR_NOT_READY = 0;
static bool StationCaptured = false;
static int PseudoRequest = 0;
static uint16_t totalRotationTime;

//Color is 0 implies, RED
//Color is 1, implies BLUE
static uint8_t myColor = 1;
static bool CapStationDontPoll = false;
//Default color is RED


/*---------------------------- ALL CITIES ---------------------------*/
#define SACRAMENTO 1
#define SEATTLE 2
#define BILLINGS 3
#define DENVER 4
#define DALLAS 5
#define CHICAGO 6
#define MIAMI 7
#define WASHINGTON 8
#define CONCORD 9


// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

//Related to getting direction
static uint8_t LocationList[9] = {SACRAMENTO,
		SEATTLE, BILLINGS, DENVER, CHICAGO, CONCORD,
		WASHINGTON, MIAMI, DALLAS};

static uint8_t PreviousLocation = 0;
static bool IsClockwise; //If this is true, then the bot is traversing in clockwise orientation


#define STATUS_POLLING_TIME_START_GAME 500//500 ms
#define STATUS_POLLING_DURING_GAME 10000  //Once in 10 s
#define HARDCORE_TIMER 1000
#define START_FOLLOWING_AGAIN 600
#define WAIT_FOR_REQUEST_DELAY 400
#define MAX_WAIT_TIME_ON_STOP 3000
#define PSEUDO_REQUEST_TIME 20

//#define DEBUG_PRINT

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateFSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitStrategySM ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
	
	
	//initialize all the pins
	initialize_all();
	
	//EDIT This Code!!
	//Read some pin on the Tiva here
	//myColor  = 0;
	
	myColor = getPartySwitch();	
	
	#ifdef DEBUG_PRINT
	printf("Part switch: %d\r\n",myColor);
	#endif
	
	//Stop motor at start
	StopMotors();
	#ifdef DEBUG_PRINT
	printf("All initializations Done!\r\n");
	#endif
	
	
    CurrentState = InitPState;
	
	//Initialize Get Status timer
	//This timer will poll the game 
	//ever 100 ms
	ES_Timer_InitTimer(STATUS_TIMER, STATUS_POLLING_TIME_START_GAME);
	

    
	//printf("Hall DISABLED!\r\n");
	disableHall();
	
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      return true;
  }else
  {
      return false;
  }
	
	
}

/****************************************************************************
 Function
     PostTemplateFSM

 Parameters
     EF_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostStrategySM( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateFSM

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event RunStrategySM( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
	ES_Event MyEvent;
	uint8_t currCode;
	uint8_t result;
	uint8_t gameStatus;
  switch ( CurrentState )
  {
    case InitPState :       // If current state is initial Psedudo State
        if ( ThisEvent.EventType == ES_INIT )// only respond to ES_Init
        {
            // this is where you would put any actions associated with the
            // transition from the initial pseudo-state into the actual
            // initial state

            // now put the machine into the actual initial state
            CurrentState = WaitingForCommand;
						

        }
        break;

    case WaitingForCommand :       // If current state is state one
			
			
      switch ( ThisEvent.EventType )
      {
            case STOP_NOW_CHANGE_TO_CAPTURE:
                MyEvent.EventType = STOP_NOW_CHANGE_TO_CAPTURE;
                PostCampaigningSM(MyEvent);
                #ifdef DEBUG_PRINT
                printf("\rStopping now in Capture mode state!\r\n");
                #endif
                break;
						
            case GET_KEYPRESS_STATUS:
                //Post to Status PAC and wait for rsesult						
                MyEvent.EventType = RETURN_STATUS;
                PostStatusPAC(MyEvent);
                #ifdef DEBUG_PRINT
                printf("\rGetting Status!\r\n");
                #endif
                break;
            case PAC_COMM_COMPLETE:
                //Received status from the status PAC
                //Do processing and formulate strategy
                #ifdef DEBUG_PRINT
                printf("\r Status request Completed!\r\n");
                #endif
            
                gameStatus = processStatusResponse(ThisEvent.EventArray);
        
                
                if(startedGame == false)
                {
                
                    if(gameStatus == 1)
                    {
                        #ifdef DEBUG_PRINT
                        printf("\r Game has started now!\r\n");
                        #endif
                        
                        //Turn on LED
                        setDisplayLED();
                                                
                        //Start campaigning
                        MyEvent.EventType = FOLLOW_LINE;
                        PostStrategySM(MyEvent);
                        startedGame = true;
                        #ifdef DEBUG_PRINT
                        printf("Hall ENABLED!\r\n");
                        #endif
                        enableHall();
                        
                        //Start big tiemer
                        ES_Timer_InitTimer(BIG_TIMER,BIG_TIMER_COUNT);
    
                    }
                    else
                    {
                        //Turn off LED
                        turnOffLED();
                        
                        //Just stay put, like a good dog
                        StopMotors();
                    }
                }
                else if(startedGame == true)
                {
                    if(gameStatus == 0)
                    {
                        #ifdef DEBUG_PRINT
                        printf("\rGame over!\r\n");
                        #endif
                        StopMotors();
                        startedGame = false;
                    }
                                
                }
                break;
            
            case FOLLOW_LINE:
					
                MyEvent.EventType = START_FOLLOWING;
                PostCampaigningSM(MyEvent);
                printf("Started following line again!\r\n");
                #ifdef DEBUG_PRINT
                printf("\rFollowing line!\r\n");
                #endif

                //Enable Capture
                //enableHall();
                break;
				
			case SHOOTING_COMPLETE:
						
                #ifdef DEBUG_PRINT
                printf("Shooting complete bitch!\r\n");
                #endif
                      
                totalRotationTime = ThisEvent.EventParam;
                //Poll status and figure out if ball went in
                MyEvent.EventType = RETURN_STATUS;
                PostStatusPAC(MyEvent);
                ShootingComplete = true;
                
                rotateClockwise();
                
                disableHall();
                ES_Timer_InitTimer(ROTATE_AFTER_SHOOT_TIMER,totalRotationTime);
                                    
                //Reset location
                clearLOC();
                
                ShootingInProgress = false;
						
				break;
             case POTENTIAL_POLL_STATION:
                
                CapStationDontPoll = true;
                currCode = getPollingStationCode();
        
                if(firstTimeFrequencyFound == false)
                {
                    firstTimeFrequencyFound = true;
                    
                    //Start timer
                    ES_Timer_InitTimer(MAX_WAIT_TIMER, MAX_WAIT_TIME_ON_STOP);
                }
                
                            
                //check frequency read and decide'
                if (currCode <=15 && (StationCaptured == false))
                {
                    
                    //Found one of the polling stations
                    //Post to Capture station
                    #ifdef DEBUG_PRINT
                    printf("\rPotential station found!\r\n");
                    #endif
                    ES_Event ThisEvent;
                    ThisEvent.EventParam = currCode;
                    ThisEvent.EventType = CAPTURE_STATION_NOW;
                    PostCampaigningSM(ThisEvent);
                    
                    //Shift to new state
                    CurrentState = WaitingUntilRequestDelayIsOver;
                    
                    //Start a timer for 200 ms until next request
                    ES_Timer_InitTimer(REQUEST_DELAY_TIMER, WAIT_FOR_REQUEST_DELAY);
                                
                }
                else
                {
                    #ifdef DEBUG_PRINT
                    printf("Hall ENABLED!\r\n");
                    #endif
                    enableHall();
                }
                break;
            case CAPTURE_STATION_RESULT:
						//Get the response
						result = processQueryResponse(ThisEvent.EventArray);
				
						#ifdef DEBUG_PRINT
						printf("\rRESULT IS..........: %d\r\n",result);
						#endif
				
						switch(result)
						{
							case 0:
                                    //Response not ready yet,
                                    //keep polling
                                    #ifdef DEBUG_PRINT
                                    printf("Hall ENABLED!\r\n");
                                    #endif
                                    enableHall();
                                    break;
							case 1:
									//NACK received, try again!
				
									NACK_COUNT++;
									if(NACK_COUNT >70)
									{
                                        //Stop wasting time and move on to another station!
                                        //Resume checking status
                                        CapStationDontPoll = false;
                                        MyEvent.EventType = FOLLOW_LINE;
                                        PostStrategySM(MyEvent);
                                        #ifdef DEBUG_PRINT
                                        printf("Hall DISABLED!\r\n");
                                        #endif
                                        disableHall();
                                        NACK_COUNT = 0;
                                        
                                        printf("Called CITY TIMER FROM NAC COUNT\r\n");
                                        ES_Timer_InitTimer(CITY_START_TIMER,START_FOLLOWING_AGAIN);
									}
									else
									{
										#ifdef DEBUG_PRINT
										printf("Hall ENABLED!\r\n");
										#endif
										enableHall();
									}
									break;
							case 2:
                                
                                //Finding Direction
                                 if(LocationUpdated == false)
                                 {
                                    uint8_t IndexA = 9;
                                    uint8_t IndexB = 9;
                                    uint8_t CurrLocation = getLOC();
                                    for(int i = 0; i < 9; i += 1) {
                                        if(CurrLocation == LocationList[i]) {
                                            IndexA = i;
                                        }
                                        if(PreviousLocation == LocationList[i]) {
                                            IndexB = i;
                                        }
                                    }
                                    if(IndexA < 9 && IndexB < 9) {
                                        if(IndexA > IndexB && IndexA != 0) {
                                            IsClockwise = true;
                                        } else if(IndexB > IndexA && IndexB != 0) {
                                            IsClockwise = false;
                                        } else if(IndexA == 0 && IndexB == 8) {
                                            IsClockwise = true;
                                        } else if(IndexB == 0 && IndexA == 8) {
                                            IsClockwise = false;
                                        }
                                    }
                                    PreviousLocation = CurrLocation;
                                    
                                    LocationUpdated = true;
                                    
                                }                          
                                //ACK received! Station shall be captured soon

                                if((getRBN()== 2 && myColor == 0) || (getRBN()== 1 && myColor == 1))
                                {
                                    //Station captured! Go forward
                                    //Stop wasting time and move on to another station!
                                    #ifdef DEBUG_PRINT
                                    printf("CAPTUUUUUUUUUUURED!\r\n");
                                    #endif
                                    
                                    //Check if station is Seattle or Miami
                                    if((getLOC() == SEATTLE_CODE) && myColor ==1)
                                    {
                                        //If I am blue, I shoot at seattle
                                        //Start shooting
                                        #ifdef DEBUG_PRINT
                                        printf("\rStart shooting now!\r\n");
                                        #endif
                                        
                                        if(ShootingInProgress == false)
                                        {
                                        
                                            MyEvent.EventType = ACTIVATE_SHOOTER;
                                            
                                            PostCampaigningSM(MyEvent);
                                            disableHall();
                                            #ifdef DEBUG_PRINT
                                            printf("Hall DISABLED!\r\n");
                                            #endif
                                            NACK_COUNT = 0;
                                            
                                            
                                            
                                            //Can start polling again
                                            CapStationDontPoll = false;
                                            ShootingInProgress = true;
                                            
                                            clearLOC();
                                        }
                                        
                                    }
                                    else if((getLOC() == MIAMI_CODE) && myColor ==0)
                                    {
                                        if(ShootingInProgress == false)
                                        {
                                            //If I am red, I shoot at miami
                                            //Start shooting
                                            #ifdef DEBUG_PRINT
                                            printf("\rStart shooting now!\r\n");
                                            #endif
                                            
                                            MyEvent.EventType = ACTIVATE_SHOOTER;
                                            
                                            PostCampaigningSM(MyEvent);
                                            
                                            disableHall();
                                            #ifdef DEBUG_PRINT
                                            printf("Hall DISABLED!\r\n");
                                            #endif
                                            NACK_COUNT = 0;
                                            
                                            //Can start polling again
                                            CapStationDontPoll = false;
                                            ShootingInProgress = true;
                                            
                                            clearLOC();
                                        }
                                        
                                    }
                                    else
                                    {
                                        //Follow the line
                                        #ifdef DEBUG_PRINT
                                        printf("\rSTATION CAPTURRRED!\r\n");
                                        #endif
                                        CapStationDontPoll = false;
                                        disableHall();
                                        #ifdef DEBUG_PRINT
                                        printf("Hall DISABLED!\r\n");
                                        #endif
                                        MyEvent.EventType = FOLLOW_LINE;
                                        PostStrategySM(MyEvent);			
                                        StationCaptured = true;
                                        ES_Timer_InitTimer(CITY_START_TIMER,START_FOLLOWING_AGAIN); //200 ms timeout to re-enable Hall
                                        firstTimeFrequencyFound = false;
                                        ShootingInProgress = false;
                                        NACK_COUNT = 0;
                                    }
                                }
                                else
                                {
                                    //Continue claiming
                                    #ifdef DEBUG_PRINT
                                    printf("Hall ENABLED!\r\n");
                                    #endif
                                    enableHall();
                                }
                                
                                break;
							
							case 3:
									//Station is blocked
									//Don't waste time, move forward
									//Resume checking status
									CapStationDontPoll = false;
									MyEvent.EventType = FOLLOW_LINE;
									PostStrategySM(MyEvent);
									disableHall();
									#ifdef DEBUG_PRINT
									printf("Hall DISABLED!\r\n");
									#endif
									printf("Called CITY TIMER FROM STATION BLOCKED\r\n");
									ES_Timer_InitTimer(CITY_START_TIMER,START_FOLLOWING_AGAIN); //200 ms timeout to re-enable Hall	

									NACK_COUNT = 0;
									break;
							
							case 4:
									//Station is busy, keep trying
									NACK_COUNT = 0;
									#ifdef DEBUG_PRINT
									printf("Hall ENABLED!\r\n");
									#endif
									enableHall();
									break;	
						}					
						break;
				
				case SHOOT_NOW:
						
						if(ShootingInProgress == false)
						{
							MyEvent.EventType = ACTIVATE_SHOOTER;
							PostCampaigningSM(MyEvent);
							#ifdef DEBUG_PRINT
							printf("\rShooting now!\r\n");
							#endif
							ShootingInProgress = true;
						}
						
						break;
				
				case ES_TIMEOUT:
					
                        if(ThisEvent.EventParam == CITY_START_TIMER)
                        {				
                            //Enable Hall sensor again
                            #ifdef DEBUG_PRINT
                            printf("Hall  RE-ENABLED!\r\n");
                            #endif
                            enableHall();
                        
                    
                            
                            //Set captured station to false
                            StationCaptured = false;
                            
                            
                            //Need this
                            ShootingInProgress = false;
                            
                            
                            //Location update
                            LocationUpdated = false;
                                                
                        }
                        
                        else if(ThisEvent.EventParam == PSEUDO_REQUEST_TIMER)
                        {
                            if(PseudoRequest == 3)
                            {
                                PseudoRequest = 0;
                                //Don't initialize timer now
                                
                            }
                            else
                            {
                                //Send a pseudo request
                                ES_Event MyEvent;
                                MyEvent.EventType = RETURN_PSEUDO_QUERY;
                                PostStatusPAC(MyEvent);
                                
                                PseudoRequest++;
                            }
                            
                        }
                        else if(ThisEvent.EventParam == STATUS_TIMER)
                        {
                            if(CapStationDontPoll == false)
                            {
                                
                                //Get status of the game
                                MyEvent.EventType = RETURN_STATUS;
                                PostStatusPAC(MyEvent);
                                                            
                            }
                                if(startedGame == false)
                                {
                                    #ifdef DEBUG_PRINT
                                    printf("\rFirst timer went off!\r\n");
                                    #endif
                                    //Reinitialize Initialize timer
                                    ES_Timer_InitTimer(STATUS_TIMER,STATUS_POLLING_TIME_START_GAME);
                                }
                                else
                                {
                                    //Reinitialize Status timer to poll
                                    ES_Timer_InitTimer(STATUS_TIMER,STATUS_POLLING_DURING_GAME);
                                }
                            
                        }
                        else if(ThisEvent.EventParam == BIG_TIMER)
                    {
                        BigTimerCount++;
                        if(BigTimerCount == 4)
                        {
                            //Stop motors, ame inactive
                            StopMotors();

                            
                            turnOffLED();
                            
                        }
                        else
                        {
                            //Initilize tiemr
                            ES_Timer_InitTimer(BIG_TIMER,BIG_TIMER_COUNT);
                        }
                    }
                        else if(ThisEvent.EventParam == ROTATE_AFTER_SHOOT_TIMER)
                        {
                            //Post follow line event
                            //Get status of the game
                            MyEvent.EventType = START_FOLLOWING;
                            PostCampaigningSM(MyEvent);
                            
                            printf("FINISHED ROTATION!\r\n");
                            
                            #ifdef DEBUG_PRINT
                            printf("Done with rotation the other way!\r\n");
                            printf("Start following the line now\r\n");
                            #endif
                            
                            //Start timeout to re-enable Hall
                            printf("Called CITY TIMER FROM ROATATE AFTER SHOOT\r\n");
                            ES_Timer_InitTimer(CITY_START_TIMER,START_FOLLOWING_AGAIN);
                        }
                        else if(ThisEvent.EventParam == MAX_WAIT_TIMER)
                        {
                            if(firstTimeFrequencyFound == true)
                            {
                                firstTimeFrequencyFound = false;
                            }
                            
                            
                            if(ShootingInProgress ==  false)
                            {
                                //Follow line anyways
                                MyEvent.EventType = FOLLOW_LINE;
                                PostStrategySM(MyEvent);
                                
                                //disable Hall
                                disableHall();
                                
                                
                                //Start City timer
                                printf("CITY_START_TIMER FROM MAX WAIT TIMER!\r\n");
                                ES_Timer_InitTimer(CITY_START_TIMER, START_FOLLOWING_AGAIN);
                            }

                            

                            //Move back to waiting
                            CurrentState = WaitingForCommand;						
                        }
                    break;
							
        default:
					break;
				
      }  // end switch on CurrentEvent
      break;
		
		case  WaitingUntilRequestDelayIsOver:
					
						if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == REQUEST_DELAY_TIMER)
						{
							#ifdef DEBUG_PRINT
							printf("Inside WaitingUntilRequestDelayIsOver switching back\n\r");
							#endif
							//Switch back to old state
							CurrentState = WaitingForCommand;
							
							
							
							if(StationCaptured == false)
							{
								//Enable Hall so that you can start receiving requests again
								#ifdef DEBUG_PRINT
								printf("Hall ENABLED!\r\n");
								#endif
								enableHall();
							}
													
						}
						else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == MAX_WAIT_TIMER)
						{
								if(firstTimeFrequencyFound == true)
								{
									firstTimeFrequencyFound = false;
								}
								
								//Follow line anyways
								//Disable Hall and follow line
								if(ShootingInProgress ==  false)
								{
									disableHall();
								
								MyEvent.EventType = FOLLOW_LINE;
								PostStrategySM(MyEvent);
								
									//Start City timer
									printf("CITY_START_TIMER FROM MAX WAIT TIMER!\r\n");
									ES_Timer_InitTimer(CITY_START_TIMER, START_FOLLOWING_AGAIN);
								}
								

								//Move back to waiting
								CurrentState = WaitingForCommand;
						}
                        else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == BIG_TIMER)
                        {
                            BigTimerCount++;
                            if(BigTimerCount == 4)
                            {
                                //Stop motors, ame inactive
                                StopMotors();
                                
                                turnOffLED();
                                
                            }
                            else
                            {
                                //Initilize tiemr
                                ES_Timer_InitTimer(BIG_TIMER,BIG_TIMER_COUNT);
                            }
                        }
						else if(ThisEvent.EventType == CAPTURE_STATION_RESULT) {
						//Get the response
						result = processQueryResponse(ThisEvent.EventArray);            
				
						switch(result)
						{
							case 0:
									//Response not ready yet,
									//keep polling
									#ifdef DEBUG_PRINT
									printf("Hall ENABLED!\r\n");
									#endif
									enableHall();
									break;
							case 1:
									//NACK received, try again!
								
									NACK_COUNT++;
							
									if(NACK_COUNT >70)
									{
										//Stop wasting time and move on to another station!
										//Resume checking status
										CapStationDontPoll = false;
										MyEvent.EventType = FOLLOW_LINE;
										PostStrategySM(MyEvent);
										#ifdef DEBUG_PRINT
										printf("Hall DISABLED!\r\n");
										#endif
										disableHall();
										NACK_COUNT = 0;
																	
										ES_Timer_InitTimer(CITY_START_TIMER,START_FOLLOWING_AGAIN); //200 ms timeout to re-enable Hall
									}
									else
									{
										#ifdef DEBUG_PRINT
										printf("Hall ENABLED!\r\n");
										#endif
										enableHall();
									}
									break;
							case 2:
									#ifdef DEBUG_PRINT
									printf("Acknowledge received\n\r");
									#endif

							#ifdef DEBUG_PRINT
							printf("GET RBN VALUE IS: %d\r\n",getRBN());
							#endif
                                    //Finding Direction
                                     if(LocationUpdated == false)
                                     {
										uint8_t IndexA = 9;
										uint8_t IndexB = 9;
										uint8_t CurrLocation = getLOC();
										for(int i = 0; i < 9; i += 1) {
											if(CurrLocation == LocationList[i]) {
												IndexA = i;
											}
											if(PreviousLocation == LocationList[i]) {
												IndexB = i;
											}
										}
										if(IndexA < 9 && IndexB < 9) {
											if(IndexA > IndexB && IndexA != 0) {
												IsClockwise = true;
											} else if(IndexB > IndexA && IndexB != 0) {
												IsClockwise = false;
											} else if(IndexA == 0 && IndexB == 8) {
												IsClockwise = true;
											} else if(IndexB == 0 && IndexA == 8) {
												IsClockwise = false;
											}
										}
										PreviousLocation = CurrLocation;
                                        
                                        LocationUpdated = true;
                                        
                                    }
                                
									if((getRBN()== 2 && myColor == 0) || (getRBN()== 1 && myColor == 1))
									{
										//Station captured! Go forward
										//Stop wasting time and move on to another station!
										#ifdef DEBUG_PRINT
										printf("Captured!\r\n");
										#endif
										
										
										//Check if station is Seattle or Miami
										if((getLOC() == SEATTLE_CODE)  && myColor == 1)
										{
											//Start shooting
											#ifdef DEBUG_PRINT
											printf("\rReached Miami or Seattle!\r\n");
											printf("\rStart shooting now!\r\n");
											#endif
											
											if(ShootingInProgress == false)
											{
											
												MyEvent.EventType = ACTIVATE_SHOOTER;
												PostCampaigningSM(MyEvent);
												disableHall();
												#ifdef DEBUG_PRINT
												printf("Hall DISABLED!\r\n");
												#endif
												NACK_COUNT = 0;
												
												//Can start polling again
												CapStationDontPoll = false;
												ShootingInProgress = true;
                                                clearLOC();
											}
											
											
										}
										else if((getLOC() == MIAMI_CODE) && myColor ==0)
										{
											if(ShootingInProgress == false)
											{
												//If I am red, I shoot at miami
												//Start shooting
												#ifdef DEBUG_PRINT
												printf("\rStart shooting now!\r\n");
												#endif
												
												MyEvent.EventType = ACTIVATE_SHOOTER;
												PostCampaigningSM(MyEvent);
												disableHall();
												#ifdef DEBUG_PRINT
												printf("Hall DISABLED!\r\n");
												#endif
												NACK_COUNT = 0;
												
												//Can start polling again
												CapStationDontPoll = false;
												ShootingInProgress = true;
                                                clearLOC();
											}
											
										}
										else
										{
											//Follow the line
											#ifdef DEBUG_PRINT
											printf("\rCaptured!\r\n");
											#endif
											CapStationDontPoll = false;
											disableHall();
											#ifdef DEBUG_PRINT
											printf("Hall DISABLED!\r\n");
											#endif
											MyEvent.EventType = FOLLOW_LINE;
											PostStrategySM(MyEvent);			
											StationCaptured = true;
											firstTimeFrequencyFound = false;
											printf("Called CITY TIMER FROM STATION CAPTURED\r\n");
											ES_Timer_InitTimer(CITY_START_TIMER,START_FOLLOWING_AGAIN); 
											NACK_COUNT = 0;
										}
									}
									else
									{
										//Continue claiming
										#ifdef DEBUG_PRINT
										printf("Hall ENABLED!\r\n");
										#endif
										enableHall();
									}
									break;
							
							case 3:
									//Station is blocked
									//Don't waste time, move forward
									//Resume checking status
									CapStationDontPoll = false;
									MyEvent.EventType = FOLLOW_LINE;
									PostStrategySM(MyEvent);
									disableHall();
									#ifdef DEBUG_PRINT
									printf("Hall DISABLED!\r\n");
									#endif
									
									printf("Called CITY TIMER FROM STATION BLOCKED\r\n");
									ES_Timer_InitTimer(CITY_START_TIMER,START_FOLLOWING_AGAIN); //200 ms timeout to re-enable Hall	

									NACK_COUNT = 0;
									break;
							
							case 4:
									//Station is busy, keep trying
									NACK_COUNT = 0;
									printf("Hall ENABLED!\r\n");
									enableHall();
									break;	
						}
						CurrentState = WaitingForCommand;
					}											
					else if(ThisEvent.EventType == FOLLOW_LINE)
					{
						MyEvent.EventType = START_FOLLOWING;
						PostCampaigningSM(MyEvent);
						#ifdef DEBUG_PRINT
						printf("\rFollowing line!\r\n");
						#endif

						//Enable Capture
						//enableHall();
						CurrentState = WaitingForCommand;
					}
    default :
      ;
  }                                   // end switch on Current State
  return ReturnEvent;
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
     J. Edward Carryer, 10/23/11, 19:21
****************************************************************************/
MyState_t QueryStrategySM ( void )
{
   return(CurrentState);
}

/***************************************************************************
 private functions
 ***************************************************************************/
uint8_t getPollingStationCode(void)
{
	//Call function from init_all module
	uint32_t pulsePeriod = getPeriodFromHall();
	
	#ifdef DEBUG_PRINT
	printf("\rRaw Pulse Period:%u\r\n", pulsePeriod);
	#endif
	
	float pulseValueMicros = (pulsePeriod * 25.0)/(1000);
	
	#ifdef DEBUG_PRINT
	printf("\rActual pulse Width:%f\r\n", pulseValueMicros);
	#endif
	
	if(abs(pulseValueMicros - 1333) <= errorInFrequency)
	{
		return 0;
	}
	else if(abs(pulseValueMicros - 1277) <= errorInFrequency)
	{
		return 1;
	}
	else if(abs(pulseValueMicros - 1222) <= errorInFrequency)
	{
		return 2;
	}
	else if(abs(pulseValueMicros - 1166) <= errorInFrequency)	
	{
		return 3;
	}
	else if(abs(pulseValueMicros - 1111) <= errorInFrequency)
	{
		return 4;
	}
	else if(abs(pulseValueMicros - 1055) <= errorInFrequency)
	{
		return 5;
	}
	else if(abs(pulseValueMicros - 1000) <= errorInFrequency)
	{
		return 6;
	}
	else if(abs(pulseValueMicros - 944) <= errorInFrequency)
	{
		return 7;
	}
	else if(abs(pulseValueMicros - 889) <= errorInFrequency)
	{
		return 8;
	}
	else if(abs(pulseValueMicros - 833) <= errorInFrequency)
	{
		return 9;
	}
	else if(abs(pulseValueMicros - 778) <= errorInFrequency)
	{
		return 10;
	}
	else if(abs(pulseValueMicros - 722) <= errorInFrequency)
	{
		return 11;
	}
	else if(abs(pulseValueMicros - 667) <= errorInFrequency)
	{
		return 12;
	}
	else if(abs(pulseValueMicros - 611) <= errorInFrequency)
	{
		return 13;
	}
	else if(abs(pulseValueMicros - 556) <= errorInFrequency)
	{
		return 14;
	}
	else if(abs(pulseValueMicros - 500) <= errorInFrequency)
	{
		return 15;
	}
	else
	{
		return 100;
	}
	
}	
		
uint8_t processQueryResponse(uint8_t dataArray[]) {

    ResponseReady = dataArray[2];
    RequestStatus = dataArray[3];
    ACK = RequestStatus & (BIT7HI | BIT6HI);
    RBN = RequestStatus & (BIT5HI | BIT4HI);
    LOC = RequestStatus & (BIT3HI | BIT2HI | BIT1HI | BIT0HI);


	
    if(ResponseReady == 0)
    {
        //Response nnot ready yet
        return 0;
    }
    else if(ResponseReady  == 0xAA)
    {
            if(ACK == NOT_ACKNOWLEDGED)
        {
            //Not acknowledged
            return 1;
        }
        else if(ACK == ACKNOWLEDGED)
        {
            //Acknowledged response
            return 2;
        }
            else if(ACK == BLOCKED)
        {
            //Blocked station
            return 3;
        }
        else if (ACK == BUSY)
        {
            //Busy station
            return 4;
        }
        else
        {
            return 100;
        }
    
    
    }
    
    return 150;

}


uint8_t processStatusResponse(uint8_t dataArray[])
{
	//If the 5th byte says campaigning, start moving
	SS3 = dataArray[4];
	
	///For now, whether game is running or not
	#ifdef DEBUG_PRINT
	printf("SS3 is: %d\r\n",SS3);
	#endif
	return SS3 & 0x1;
	
}

uint8_t getACK(void) {
    return ACK;
}

uint8_t getRBN(void) {

    return (RBN >> 4);
}

uint8_t getLOC(void) {
    return LOC;
}

uint8_t getMyColor(void)
{
	return myColor;
}

void clearLOC(void)
{
	LOC = 0;
}

