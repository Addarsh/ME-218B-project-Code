//Module to initialize all modules of the 
// of the project at the start of the framework

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Events.h"
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

#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"
#include "strategy.h"

#include "init_all.h"
#include "PWM10Tiva.h"
#include "MotorDrive.h"
#include "Campaigning.h"
#include "Direction.h"
#include "BeaconClockWiseOrder.h"


//#define DEBUG_INTERRUPTS

//Define some Constants
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
#define BitsPerNibble 4
#define NanoSecPerSec 1000000000
#define SEATTLE_FREQUENCY 1450
#define CONCORD_FREQUENCY 1700
#define MIAMI_FREQUENCY 1250
#define SACRAMENTO_FREQUENCY 1950

#define ERROR_IN_FREQUENCY 5

#define ALL_BITS (0xff << 2)
#define NUM_SAMPLES 8
#define IR_COUNTER 5
#define IR_COUNTER_S 10
static int MiamiBeaconCounter = 0;
static int SeattleBeaconCounter = 0;


//Timers
#define TIMER_BASE 0x400FE000
#define PWRTIMER 0xA5C

//Define all static functions
static void InitializeSPI(void);
static void initHallInputPC4(void);
static void initHallInputPC6(void);
static void InitIRInputCapture(void);
static void InitUltrasonic( void );
static void init_PWM(void);
static void enableUltrasonic(void);
static void disableUltrasonic(void);
static void initPortE(void);
static void initPortD(void);
static uint32_t calculateMovingAverageA(void);
static uint32_t calculateMovingAverageB(void);
static bool startedShoot = false;

uint32_t getFrequencyA(void);
uint32_t getFrequencyB(void);


//Module variables
static bool ATriggeredFirst = false;
static bool BTriggeredFirst = false;
static bool seattleDetected = false;
static bool miamiDetected = false;
static bool concordDetected = false;
static bool sacramentoDetected = false;



//Variable to set direction
static Direction CurrentDirection;


void initialize_all(void)
{
	InitializeSPI();
	
	initHallInputPC4();
	initHallInputPC6();
	
	init_PWM();
	
	
	
	//initialize IR sensors on PD0
	InitIRInputCapture();
	
	
	
	//Initialize ultrasonic
	initPortE();
	
	initPortD();
	
	InitUltrasonic();
	
	
	
	
	//Enable global interrupts
	__enable_irq();
	
	
	//Set Current direction
	CurrentDirection = DontKnow;
	
}


void init_PWM(void)
{
	//Initialize PWM modules
	//PB6 and PB7 are right and left motor drive channels respectively,
	//PB4 is the camera servo, PB5 is the launcher servo.
	PWM_TIVA_Init(4);
	PWM_TIVA_SetFreq(3000, 0); //Group 0 PWM frequency (motors)
	PWM_TIVA_SetFreq(50, 1); //Group 1 PWM frequency (servos)
}


void initHallInputPC4(void)
{
	//Use PC4 and PC5 as inputs
	
	//Enable clock to the wide timer
	//Disable the timer
	//Set timer in 32 bit mode
	//Set ILR to FFFFFFFF
	//Select Input capture in timre mode register
	//Select Timer A for capture and in rising edge mode and upcounting
	//Enable the local timer interrupt pin
	//Enable NVIC register
	//Enable global interrupt
	//Select Alternate pin functionaluty on PC 
	//Enable timer'
	
	// start by enabling the clock to the timer (Wide Timer 0)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
	
	//Wait until clock is enabled
	//We can enable clock to port C in the meanwhile
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
	
	//Disable timer A of Wide Timer 0
	// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
	
	//Disable Timer B
	HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
	
	// Set it up in 32 bit mode
	HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
	
	//Set up interval load register value to FFFFFFFF
	HWREG(WTIMER0_BASE + TIMER_O_TAILR) = 0xffffffff;
	HWREG(WTIMER0_BASE + TIMER_O_TBILR) = 0xffffffff;
	
	//Set timer to input capture (set 2 bits to 0), edge time mode and upcounting
	//Set TAMR = 3 and TAAMS = 0
	//CAP - capture register
	//CMR = edge time register
	//CDIR = up counting direction of timer
	HWREG(WTIMER0_BASE+TIMER_O_TAMR) = (HWREG(WTIMER0_BASE+TIMER_O_TAMR) & ~(TIMER_TAMR_TAAMS)) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
	HWREG(WTIMER0_BASE+TIMER_O_TBMR) = (HWREG(WTIMER0_BASE+TIMER_O_TBMR) & ~(TIMER_TBMR_TBAMS)) | (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);
	
	// To set the event to rising edge, we need to modify the TAEVENT bits
	// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
	HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
	HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEVENT_M;
	
	//Now set up Pin C4, Pin C5 to alternate function
	HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= (BIT4HI|BIT5HI);
	
	 //Write 4's alternate function WT0CCP0 to the control register of Port C
	// 7 is the mux value to select WT0CCP0, 16 to shift it over to the
	// right nibble for bit 4 (4 bits/nibble * 4 bits)
	HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) =
	(HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xff00ffff) | ((7<<16)| (7<<20));
	

  // Enable pin on Pin C4 for digital I/O
	HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= BIT4HI|BIT5HI;
	
	// make pin 4 on Port C into an input
	HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= (BIT4LO & BIT5LO);
	
	// Enable a local capture interrupt
	HWREG(WTIMER0_BASE+TIMER_O_IMR) |= (TIMER_IMR_CAEIM| TIMER_IMR_CBEIM);
	
	//Enable NVIC for PC4 and PC5
	HWREG(NVIC_BASE+EN2) |= (BIT30HI);
	
	
	// now kick the timer off by enabling it and enabling the timer to
	// stall while stopped by the debugger
	HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL| TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
}

void initHallInputPC6(void)
{
	
	//Use Wide Time 1 on PC6 to enable Input capture for second hall sensor
		//Enable clock to wide timer 1
	//Disable the timer
	//Set timer in 32 bit mode
	//Set ILR to FFFFFFFF
	//Select Input capture in timer mode register
	//Select Timer A for capture and in rising edge mode and upcounting
	//Enable the local timer interrupt pin
	//Enable NVIC register
	//Enable global interrupt
	//Select Alternate pin functionaluty on PC 
	//Enable timer'
	
	// start by enabling the clock to the timer (Wide Timer 0)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R1;
	
	//Wait until clock is enabled
	//We can enable clock to port C in the meanwhile
	//Not needed, this is done for PC4
	//HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;
	
	//Disable timer A of Wide Timer 1
	// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
	
	// Set it up in 32 bit mode
	HWREG(WTIMER1_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
	
	//Set up interval load register value to FFFFFFFF
	HWREG(WTIMER1_BASE + TIMER_O_TAILR) = 0xffffffff;
	
	//Set timer to input capture (set 2 bits to 0), edge time mode and upcounting
	//Set TAMR = 3 and TAAMS = 0
	//CAP - capture register
	//CMR = edge time register
	//CDIR = up counting direction of timer
	HWREG(WTIMER1_BASE+TIMER_O_TAMR) = (HWREG(WTIMER1_BASE+TIMER_O_TAMR) & ~(TIMER_TAMR_TAAMS)) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
	
	
	// To set the event to rising edge, we need to modify the TAEVENT bits
	// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
	HWREG(WTIMER1_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;
	
	//Now set up Pin C6 to alternate function
	HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= BIT6HI;
	
	 //Write 4's alternate function WT0CCP0 to the control register of Port C
	// 7 is the mux value to select WT0CCP0, 24 to shift it over to the
	// right nibble for bit 4 (4 bits/nibble * 6 bits)
	HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) =
	(HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) & 0xf0ffffff) | (7<<24);
	

  // Enable pin on Pin C4 for digital I/O
	HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= BIT6HI;
	
	// make pin 4 on Port C into an input
	HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= BIT6LO;
	
	// Enable a local capture interrupt
	HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
	
	//ennable NVIC for Wide Timer 1
	HWREG(NVIC_BASE+EN3) |= (BIT0HI);
	
	// now kick the timer off by enabling it and enabling the timer to
	// stall while stopped by the debugger
	HWREG(WTIMER1_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
}



void InitializeSPI(void)
{
    //SSI0:
    //PA2: Clk
    //PA3: Fss
    //PA4: Rx
    //PA5: Tx
    //Enable the clock to the GPIO port
    HWREG(SYSCTL_BASE+RCGCGPIO) |= BIT0HI; // GPIO Port A Run Mode Clock Gating Control
	
	while(((HWREG(SYSCTL_PRGPIO)&SYSCTL_PRGPIO_R0)) != SYSCTL_PRGPIO_R0); //Wait until initialization completes
	//while((HWREG(SYSCTL_BASE+RCGCGPIO) & BIT0HI) == 0); //Wait until initialization completes
    
	//Enable the clock to SSI module
    HWREG(SYSCTL_BASE+RCGCSSI) |= BIT0HI; //Enable SSI Module 0 (See Table 15-1, uses Port A)
    while((HWREG(SYSCTL_BASE+RCGCSSI) & BIT0HI) == 0); //Wait for the SSI0 to be ready
    
	//Program the GPIO to use the alternate functions on the SSI pins
    HWREG(GPIO_PORTA_APB_BASE+GPIO_AFSEL) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); //A2:A5
    
	//Set mux position in GPIOPCTL to select the SSI use of the pins
    HWREG(GPIO_PORTA_APB_BASE+GPIO_PCTL) |= (BIT9HI | BIT13HI | BIT17HI | BIT21HI); //0x02 MUX value
	
	HWREG(GPIO_PORTA_APB_BASE+GPIO_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 
	HWREG(GPIO_PORTA_APB_BASE+GPIO_DIR) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 
	
    //Program the port lines for digital I/O
    HWREG(GPIO_PORTA_APB_BASE+GPIO_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 
    
	//Program the required data directions on the port lines
    HWREG(GPIO_PORTA_APB_BASE+GPIO_DIR) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 
    
	//If using SPI mode 3, program the pull-up on the clock line
    HWREG(GPIO_PORTA_APB_BASE+GPIO_PUR) |= BIT2HI; //PA2 
    
	//Make sure that the SSI is disabled before programming mode bits
    HWREG(SSI_0_BASE+SSICR1) &= BIT1LO;
    
	//Select master mode (MS) & TXRIS indicating End of Transmit (EOT)
    HWREG(SSI_0_BASE+SSICR1) &= BIT2LO; //Master select
    HWREG(SSI_0_BASE+SSICR1) |= BIT4HI; //EOT
    
	//Configure the SSI clock source to the system clock
    HWREG(SSI_0_BASE+SSICC) &= (BIT0LO & BIT1LO & BIT2LO & BIT3LO); //Use system clock (40MHz)
    
	//Configure the clock pre-scaler
    HWREG(SSI_0_BASE+SSICPSR) = 254; //Corresponds to 50 (0b00110010), want < 961kHz
    
	//Configure clock rate (SCR), phase & polarity (SPH, SPO), mode (FRF), data size (DSS)
    HWREG(SSI_0_BASE+SSICR0) |= (15 << 8);
    HWREG(SSI_0_BASE+SSICR0) |= BIT7HI; //SPH = 1
    HWREG(SSI_0_BASE+SSICR0) |= BIT6HI; //SPO = 1
    
	//SPI Frame Format => FRF set low
    HWREG(SSI_0_BASE+SSICR0) |= 0x7; //SSI Data Size = 8 bits
    
	//Locally enable interrupts (TXIM in SSIIM)
    HWREG(SSI_0_BASE+SSIIM) |= BIT3HI; //TXIM Interrupt unmasked, SHOULD THIS BE RECEIVE???
    
	//Make sure that the SSI is enabled for operation
    HWREG(SSI_0_BASE+SSICR1) |= BIT1HI; 
}

void InitIRInputCapture(void)
{
	//PD0
	//Enable clock to the wide timer
	//Disable the timer
	//Set timer in 32 bit mode
	//Set ILR to FFFFFFFF
	//Select Input capture in timer mode register
	//Select Timer A for capture and in rising edge mode and upcounting
	//Enable the local timer interrupt pin
	//Enable NVIC register
	//Enable global interrupt
	//Select Alternate pin functionality on PC
	//Enable timer'

	// start by enabling the clock to the timer (Wide Timer 2)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R2;

	//Wait until clock is enabled
	//We can enable clock to port D in the meanwhile
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;

	//Disable timer A of Wide Timer 2
	// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;

	//Disable Timer B
	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;

	// Set it up in 32 bit mode
	HWREG(WTIMER2_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;

	//Set up interval load register value to FFFFFFFF
	HWREG(WTIMER2_BASE + TIMER_O_TAILR) = 0xffffffff;

	//Set timer to input capture (set 2 bits to 0), edge time mode and upcounting
	//Set TAMR = 3 and TAAMS = 0
	//CAP - capture register
	//CMR = edge time register
	//CDIR = up counting direction of timer
	HWREG(WTIMER2_BASE+TIMER_O_TAMR) = (HWREG(WTIMER2_BASE+TIMER_O_TAMR) & ~(TIMER_TAMR_TAAMS)) | (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);

	// To set the event to rising edge, we need to modify the TAEVENT bits
	// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
	HWREG(WTIMER2_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;

	//Now set up PD0 to alternate function
	HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT0HI;

	 //Write 4's alternate function WT2CCP0 to the control register of Port D
	// 7 is the mux value to select WT2CCP0, 0 to shift it over to the
	// right nibble for bit 0 (4 bits/nibble * 0 bits)
	HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffffff0) | (7<<0);


  // Enable PD0 for digital I/O
	HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT0HI;

	// make PD0 into an input
	HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT0LO;

	// enable NVIC 98: EN3, bit 2
	HWREG(NVIC_EN3) |= BIT2HI;
	// make sure interrupts are enabled globally

	
	// now kick the timer off by enabling it and enabling the timer to
	// stall while stopped by the debugger
	HWREG(WTIMER2_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);
}


/*********************************************************
Hall ISR fr PC4 and PC6 and related modules
***********************************************************/

static uint32_t pulsePeriodA = 0;
static int counterA = 0;
static uint32_t newEdgeTimeA = 0;
static uint32_t oldEdgeTimeA = 0;
static uint32_t MovingAverageA[NUM_SAMPLES] = {0};
static uint8_t avgCountA = 0;

void ISRHallPC4(void)
{
	
	//Clear interrupt source
	HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
	

	counterA++;
	
	newEdgeTimeA = HWREG(WTIMER0_BASE+TIMER_O_TAR);
	pulsePeriodA = newEdgeTimeA - oldEdgeTimeA;
	oldEdgeTimeA = newEdgeTimeA;
	
	
	if(counterA > 10)
	{
		//stop motors
		UpdatePWM(50,50);

	}
	
	if(avgCountA < NUM_SAMPLES)
	{
		MovingAverageA[avgCountA] = pulsePeriodA;
	}
	else
	{
		avgCountA = 0;
		//Calculate average
		
		//Send an event to strategy
		ES_Event ThisEvent;
		ThisEvent.EventType = POTENTIAL_POLL_STATION;
		PostStrategySM(ThisEvent);
		ATriggeredFirst = true;
		disableHall();
		
	}
	avgCountA++;
	
	#ifdef DEBUG_INTERRUPTS
	printf("Firing hall\r\n");
	#endif
	
}

static uint32_t pulsePeriodB = 0;
static int counterB =  0;
static uint8_t avgCountB = 0;
static uint32_t newEdgeTimeB = 0;
static uint32_t oldEdgeTimeB = 0;
static uint32_t MovingAverageB[NUM_SAMPLES] = {0};

void ISRHallPC6(void)
{
	
	
	//Clear interrupt source
	HWREG(WTIMER1_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;
	
	newEdgeTimeB = HWREG(WTIMER1_BASE+TIMER_O_TAR);
	pulsePeriodB = newEdgeTimeB - oldEdgeTimeB;
	oldEdgeTimeB = newEdgeTimeB;
	
	counterB++;

	
	if(counterB > 10)
	{
		//Stop motors
		UpdatePWM(50,50);
	
	}
	
	if(avgCountB < NUM_SAMPLES)
	{
		//printf("AVGB: %d\r\n",avgCountB);
		MovingAverageB[avgCountB] = pulsePeriodB;
	}
	else
	{
		avgCountB = 0;
		//Calculate average
		
		//Send an event to strategy
		ES_Event ThisEvent;
		ThisEvent.EventType = POTENTIAL_POLL_STATION;
		PostStrategySM(ThisEvent);
		BTriggeredFirst = true;
		disableHall();
	}
	
	avgCountB++;
	
	#ifdef DEBUG_INTERRUPTS
	printf("Firing hall\r\n");
	#endif
	
}



uint32_t calculateMovingAverageA(void)
{
	uint32_t averageA = 0;
	for(int i =1; i < NUM_SAMPLES;i++)
	{
		averageA += MovingAverageA[i];
		
	}

	return averageA/(NUM_SAMPLES-1);
}

uint32_t calculateMovingAverageB(void)
{
	uint32_t averageB = 0;
	for(int i =1; i < NUM_SAMPLES;i++)
	{
		averageB += MovingAverageB[i];
	}
	
		
	return averageB/(NUM_SAMPLES-1);
}


uint32_t getPeriodFromHall(void)
{
	//return pulse value from Hall that triggered first
	if(ATriggeredFirst)
	{
		uint32_t resultA = calculateMovingAverageA();
		return resultA;
	}
	else if(BTriggeredFirst)
	{
		uint32_t resultB = calculateMovingAverageB();
		return resultB;
	}
	else
	{
		#ifdef DEBUG_INTERRUPTS
		printf("WTF, Hall pulse reading logic buggy!\r\n");
		#endif
		return 0;
	}
		
}

uint32_t getPulsePeriodA(void)
{
	return  pulsePeriodA;
}

uint32_t getPulsePeriodB(void)
{
	return  pulsePeriodB;
}


void disableHall(void)
{
	//HWREG(NVIC_BASE+EN2) &= (BIT30LO & BIT31LO);
	HWREG(WTIMER0_BASE+TIMER_O_IMR) &= (~(TIMER_IMR_CAEIM)&(~TIMER_IMR_CBEIM));
	HWREG(WTIMER1_BASE+TIMER_O_IMR) &= ~TIMER_IMR_CAEIM;
	
	counterA = 0;
	counterB = 0;
	
}
void enableHall(void)
{
	HWREG(WTIMER0_BASE+TIMER_O_IMR) |= (TIMER_IMR_CAEIM| TIMER_IMR_CBEIM);
	// Enable a local capture interrupt
	HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
	//HWREG(NVIC_BASE+EN2) |= (BIT30HI);
	
	counterA = 0;
	counterB = 0;
	
	newEdgeTimeB = 0;
	oldEdgeTimeB = 0;
	newEdgeTimeA = 0;
	oldEdgeTimeA = 0;
	pulsePeriodA = 0;
	pulsePeriodB = 0;
	
	avgCountA = 0;
	avgCountB = 0;
	for(int i =0; i < NUM_SAMPLES;i++)
	{
		MovingAverageA[i] = 0;
		MovingAverageB[i] = 0;
	}

	
	ATriggeredFirst = false;
	BTriggeredFirst = false;
}


/*----------ISR for Input Capture of IR receiver
 This capture is on PD0.---------------------*/
static uint32_t Period;
static uint32_t LastCapture = 0;
static int IRcounter = 0;

void IRInputCapture(void) {
	uint32_t ThisCapture;
	
	//printf("\rIR firing!\r\n");
	//Clear the interrupt on PD0
	HWREG(WTIMER2_BASE+TIMER_O_ICR) = TIMER_ICR_CAECINT;

    // grab the captured value and calculate the period
    ThisCapture = HWREG(WTIMER2_BASE+TIMER_O_TAR);
    // calculate the current period
    Period = ThisCapture - LastCapture;
    // update LastCapture to prepare for the next edge
    LastCapture = ThisCapture;
	
	//Check if frequency is around 1450 for seattle
	// and around 1250 Hz for Miami
	uint32_t myFrequency = NanoSecPerSec/(Period*25);

	
	
	if( (abs(MIAMI_FREQUENCY - myFrequency)<= ERROR_IN_FREQUENCY) && (getMyColor() == 0))
	{
		MiamiBeaconCounter++;
		
		if(MiamiBeaconCounter > IR_COUNTER)
		{
			//I am red!
			//Shoot at MIAMI, it is th blue bucket
	
			printf("\nStarted Shoot!\r\n");
			ES_Event ThisEvent;
			ThisEvent.EventType = ALIGN_COMPLETE;
			//Post an event to 	Campaigning
			//Should fall through to Activate Shooter stste
			PostCampaigningSM(ThisEvent);
			
			//Compute direction of rotation		
			disableIR();
			
			MiamiBeaconCounter = 0;
		}
	}
	else if((abs(SEATTLE_FREQUENCY - myFrequency)<= ERROR_IN_FREQUENCY) && (getMyColor() == 1))
	{
		SeattleBeaconCounter++;
		
		if(SeattleBeaconCounter > IR_COUNTER_S)
		{
			
			//I am blue!
			//Shoot at SEATTLE, it is th blue bucket
			//Beacon detected, stop motors, align complet
			
			ES_Event ThisEvent;
			ThisEvent.EventType = ALIGN_COMPLETE;
			//Post an event to 	Campaigning
			//Should fall through to Activate Shooter stste
			PostCampaigningSM(ThisEvent);	
			disableIR();
			SeattleBeaconCounter = 0;
		}
	
	}
	#ifdef DEBUG_INTERRUPTS
	printf("Firing IR\r\n");
	#endif
        
        
}


Direction getCurrentDirection(void)
{
	return CurrentDirection;
}


uint32_t getIRFrequency(){
	
	uint32_t myFrequency = NanoSecPerSec/(Period*25);
	#ifdef DEBUG_INTERRUPTS
	printf("My IR Frequency = %i\n\r",myFrequency);
	#endif
	return myFrequency;
}



void enableIR(void)
{
	//Enbale local interrupts
	HWREG(WTIMER2_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;
	
	startedShoot = false;

}

void disableIR(void)
{
	//Disable local interrupts
	HWREG(WTIMER2_BASE+TIMER_O_IMR) &= (~TIMER_IMR_CAEIM);

	miamiDetected = false;
	seattleDetected = false;
	concordDetected = false;
	sacramentoDetected = false;
}



/*********************************************************
Ultrasonic initialization and realted modules
***********************************************************/

void initPortE(void)
{
	// Pin PE4 used to enable the Ultrasonic trigger
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R4;
    //Manual delay to ensure clock is ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R4) != SYSCTL_PRGPIO_R4);
	
	  HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (GPIO_PIN_1)|(GPIO_PIN_2)|(GPIO_PIN_4); //Initialize Port E Bit 4 to be digital
  HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= (GPIO_PIN_1)|(GPIO_PIN_2)|(GPIO_PIN_4); //Initialize Port E Bit 4 to be output
}


void initPortD(void)
{
	// Pin PE4 used to enable the Ultrasonic trigger
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
    //Manual delay to ensure clock is ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R3) != SYSCTL_PRGPIO_R3);
	
	  HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= (GPIO_PIN_3); //Initialize Port D Bit 3 to be digital
  HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT3LO; //Initialize Port D Bit 3 to be input
}

void setDisplayLED(void)
{
	//get switch position
	int result = (HWREG(GPIO_PORTD_BASE+(GPIO_O_DATA+ALL_BITS)) & BIT3HI)>>3;
	
	if(result == 1)
	{
		//I am blue
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) |= BIT1HI;
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) &= BIT2LO;
	}
	else
	{
		//I am red
			HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) |= BIT2HI;
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) &= BIT1LO;
		
	}
}

void turnOffLED(void)
{
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) &= BIT2LO;
		HWREG(GPIO_PORTE_BASE+(GPIO_O_DATA+ALL_BITS)) &= BIT1LO;
}


uint8_t getPartySwitch(void)
{
	return (HWREG(GPIO_PORTD_BASE+(GPIO_O_DATA+ALL_BITS)) & BIT3HI)>>3;
	
}




void InitUltrasonic( void )
{
	// start by enabling the clock to the timer (Wide Timer 3)
	HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R5; 
	// enable the clock to Port D
	HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
	// since we added this Port C clock init, we can immediately start
// into configuring the timer, no need for further delay
	while((HWREG(TIMER_BASE+PWRTIMER) &  BIT5HI) == 0);
	//for(int i = 0; i < 100; i += 1);
	// make sure that timer (Timer A) is disabled before configuring
	HWREG(WTIMER5_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
// set it up in 32bit wide (individual, not concatenated) mode
// the constant name derives from the 16/32 bit timer, but this is a 32/64
	// bit timer so we are setting the 32bit mode
HWREG(WTIMER5_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
	// we want to use the full 32 bit count, so initialize the Interval Load
	// register to 0xffff.ffff (its default value :-)
HWREG(WTIMER5_BASE+TIMER_O_TAILR) = 0xffffffff;
	// set up timer A in capture mode (TAMR=3, TAAMS = 0),
	// for edge time (TACMR = 1) and up-counting (TACDIR = 1)
	HWREG(WTIMER5_BASE+TIMER_O_TAMR) =
	(HWREG(WTIMER5_BASE+TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
	(TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);
// To set the event to rising edge, we need to modify the TAEVENT bits
	// in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
	HWREG(WTIMER5_BASE+TIMER_O_CTL) |= TIMER_CTL_TAEVENT_BOTH;//****
	// Now Set up the port to do the capture (clock was enabled earlier)
	// start by setting the alternate function for Port D bit 6 (WT5CCP0)
	HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= BIT6HI;
	// Then, map bit 4's alternate function to WT4CCP0
	// 7 is the mux value to select WT5CCP0, 16 to shift it over to the
	// right nibble for bit 4 (4 bits/nibble * 6 bits)
	HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) =
	(HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xf0ffffff) + (7<<24);//****
// Enable pin on Port C for digital I/O
	HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= BIT6HI;
	// make pin 4 on Port C into an input
	HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= BIT6LO;
	// back to the timer to enable a local capture interrupt
	HWREG(WTIMER5_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM; //****
	// enable the Timer A in Wide Timer 0 interrupt in the NVIC
	// it is interrupt number 104 so appears in EN3 at bit 7
	HWREG(NVIC_EN3) |= BIT8HI; //****
	// make sure interrupts are enabled globally
	__enable_irq();
	// now kick the timer off by enabling it and enabling the timer to
	// stall while stopped by the debugger
	HWREG(WTIMER5_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL); //****
}


