//#define DEBUG_ULTRA
/****************************************************************************
 Module
  ultrasonic.c

 Revision
   1.0.1

 Description
   This is a template file for implementing a simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ultrasonic.h"
#include "ES_ShortTimer.h"
#include "ultrasonic.h"
#include "init_all.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "BITDEFS.H"

#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"

#define ALL_BITS (0Xff<<2)

#define SOS 340
#define TEN_MIL 10
#define BitsPerNibble 4

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

float querydistance(void);
void InitPWM(void);
void SetPWM(float);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint32_t Period;
//static uint32_t LastCapture;
static float distance;
static UltraState_t CurrentState= FindDistance;
static bool checker = false;
static uint32_t counter = 0;
static uint32_t risetime;
static uint32_t falltime;
#define PeriodInMS 100
#define PWMTicksPerMS 40000


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitUltra ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
    
   InitPWM();
   SetPWM(0.01);
   
   CurrentState = FindDistance; 
    
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
     PostTemplateService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostUltra( ES_Event ThisEvent )
{

  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event RunUltra( ES_Event ThisEvent )
{

  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  switch (CurrentState)
  {
 

    case FindDistance:
 
        break;
  }
  return ReturnEvent;

}

/***************************************************************************
 private functions
 ***************************************************************************/
void ISRUltraResponse( void )
{
	uint32_t ThisCapture;
	// start by clearing the source of the interrupt, the input capture event
	HWREG(WTIMER5_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT; //****
    
    if ((HWREG(GPIO_PORTD_BASE+(GPIO_O_DATA+ALL_BITS))&GPIO_PIN_6) == 64)
	{
		risetime = HWREG(WTIMER5_BASE+TIMER_O_TAR);
	}
	else
	{
		falltime = HWREG(WTIMER5_BASE+TIMER_O_TAR);
		Period = falltime - risetime;
        distance = Period*0.00000425;
        
		ES_Event SendEvent;
		SendEvent.EventType = ECHO_DETECTED;
		PostUltra(SendEvent);
	}
}

void setTriggerHI(void)
{
	HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) |= GPIO_PIN_4;
}


void setTriggerLO(void)
{
	HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) &= ~GPIO_PIN_4;
}


void enableUltrasonic(void)
{
	//Unmask raw interrupt bit
	// back to the timer to enable a local capture interrupt
	HWREG(WTIMER5_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM; //****
		
}


void disableUltrasonic(void)
{
	//Mask local interrupt
	//Unmask raw interrupt bit
	// back to the timer to enable a local capture interrupt
	HWREG(WTIMER5_BASE+TIMER_O_IMR) &= ~TIMER_IMR_CAEIM; //****
		
}

float querydistance(void)
{
	return distance;
}

void InitPWM(void){
		// initialize the PWM module

		// enable clock to PWM0 module
		HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;
		// enable clock to port E
		HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4;
		// select system clock 32
		HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) | (SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);
		// wait for the PWM clock to be set
		while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0);
		// disable the PWM
		HWREG( PWM0_BASE+PWM_O_2_CTL ) = 0;

		// PE4: M0PWM4 Generator 2
		// generator 2 comparator A with 1 at rising edge and 0 at falling edge
		HWREG( PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ZERO );
		// set the PWM period
		HWREG( PWM0_BASE+PWM_O_2_LOAD) = ((PeriodInMS * PWMTicksPerMS )>>1)/32;
		// set the initial PWM duty cycle (50%)
		HWREG( PWM0_BASE+PWM_O_2_CMPA) = HWREG(PWM0_BASE+PWM_O_2_LOAD)>>1;
		// enable the PWM outputs
		HWREG( PWM0_BASE+PWM_O_ENABLE) |= PWM_ENABLE_PWM4EN;

		// select alternate function for PE4, bit 4
		HWREG(GPIO_PORTE_BASE+GPIO_O_AFSEL) |= BIT4HI;
		// now choose to map PWM to those pins, this is a mux value of 4 that we want to use for specifying the function on bit 4
		HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTE_BASE+GPIO_O_PCTL) & 0xfff0ffff) + (4<<(4*BitsPerNibble));
		// Enable PE4 for digital I/O
		HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= BIT4HI;
		// make PE4 into outputs
		HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= BIT4HI;

		// set the up/down count mode, enable the PWM generator and make generator B updates locally synchronized to zero count
		HWREG(PWM0_BASE+ PWM_O_2_CTL) = (PWM_2_CTL_MODE | PWM_2_CTL_ENABLE | PWM_2_CTL_GENAUPD_LS);

    printf("PWM initialized \n\r");

}

void SetPWM(float DutyCycle){
        // store the maximum period to a local variable
		uint16_t PeriodInTicks;
		PeriodInTicks = HWREG( PWM0_BASE+PWM_O_2_LOAD);
		printf("Duty cycle = %f\n\r",DutyCycle);

		// set the duty cycle to 0%
		if (DutyCycle == 0) {
				HWREG( PWM0_BASE+PWM_O_2_GENA) = PWM_2_GENA_ACTZERO_ZERO;
        // set the duty cycle to 100%
		}else if (DutyCycle == 100) {
				HWREG( PWM0_BASE+PWM_O_2_GENA) = PWM_2_GENA_ACTZERO_ONE;
        // set the duty cycle to any value between 0% and 100%
		}else{
				// reset the generator mode
				HWREG( PWM0_BASE+PWM_O_2_GENA) = (PWM_2_GENA_ACTCMPAU_ONE | PWM_2_GENA_ACTCMPAD_ZERO );
				HWREG( PWM0_BASE+PWM_O_2_CMPA) = PeriodInTicks - DutyCycle * PeriodInTicks / 100;
		}

}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

#ifdef DEBUG_ULTRA
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Port.h"
#include "termio.h"

#define clrScrn() 	printf("\x1b[2J")
#define goHome()	printf("\x1b[1,1H")
#define clrLine()	printf("\x1b[K")


int main(void)
{  
	// Set the clock to run at 40MhZ using the PLL and 16MHz external crystal
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
			| SYSCTL_XTAL_16MHZ);
	TERMIO_Init();
	clrScrn();

	ES_Return_t ErrorType;


	// When doing testing, it is useful to announce just which program
	// is running.
	puts("\rStarting Test Harness for \r");
	printf("the 2nd Generation Events & Services Framework V2.2\r\n");
	printf("%s %s\n",__TIME__, __DATE__);
	printf("\n\r\n");
	printf("Press any key to post key-stroke events to Service 0\n\r");
	printf("Press 'd' to test event deferral \n\r");
	printf("Press 'r' to test event recall \n\r");

	// Your hardware initialization function calls go here
initialize_all();
	// now initialize the Events and Services Framework and start it running
	ErrorType = ES_Initialize(ES_Timer_RATE_1mS);
	if ( ErrorType == Success ) {

	  ErrorType = ES_Run();

	}
	//if we got to here, there was an error
	switch (ErrorType){
	  case FailedPost:
	    printf("Failed on attempt to Post\n");
	    break;
	  case FailedPointer:
	    printf("Failed on NULL pointer\n");
	    break;
	  case FailedInit:
	    printf("Failed Initialization\n");
	    break;
	 default:
	    printf("Other Failure\n");
	    break;
	}
	for(;;)
	  ;

}

#endif
