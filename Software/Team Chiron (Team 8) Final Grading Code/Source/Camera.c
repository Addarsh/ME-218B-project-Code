//Module for interfacing with TAOS TSL1401CL line camera

#define LOWPASS
//#define LIGHT_THRESHOLD


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
#include "Servo.h"

#include "Camera.h"

#define ALL_BITS (0Xff<<2)
#define NVIC_BASE 0xE000E000
#define EN0 0x100
#define EN1 0x104
#define EN2 0x108
#define EN3 0x10C
#define TICKS_PER_MS 40000
#define INTERRUPT_100_TO_103_PRI 0x464
#define CLKTIMER_US 30 //30
#define CAMERA_SAMPLE_US 20000 //10000
#define TICKS_PER_US 40
#define MAX_COUNT 1000
#define SI_HI BIT0HI
#define CLK_HI BIT1HI
#define SI_LO BIT0LO
#define CLK_LO BIT1LO
#define WINDOW_SIZE 3 //Oddnumber greater than 1 only
#define DEADBAND 8
#define REFERENCE 0.00

#define MIN_MAX_DIFF_THRESHOLD 600 //If MAX-MIN > 1000, then all the line found is valid

#define WTimerA 3
#define WTimerB 4

#define FORWARD_SERVO_ANGLE 45
#define REVERSE_SERVO_ANGLE 135

//Data private to the module: CurrentState, MyPriority, Buffer, LastLineCenter,
//BufferInd, CLKState, AnalogReading, min_allowed_width, max_allowed_width, 
//CompletedRead, thresholded, SI_Counter, ForwardDrive, LineFound
static CameraState_t CurrentState;
static uint8_t MyPriority;
static uint16_t Buffer[128];
static float LastLineCenter;
static uint8_t BufferInd = 0;
static bool CLKState = false;
static uint32_t AnalogReading[1];
static const uint8_t min_allowed_width = 5;
static const uint8_t max_allowed_width = 35;
static bool CompletedRead = false;
static bool thresholded[128];
static uint16_t SI_Counter = 0; // Not actually used, but this kills some time
static int Direction = 1;
static bool ForwardDrive = true;
static bool LineFound = false;


//InitializeCamera
void InitializeCamera(uint8_t priority) {
//Takes a priority number, returns nothing.
    
    //Set MyPriority to priority.
    MyPriority = priority;
    //Set CurrentState to InitPseudoState.
    CurrentState = InitPseudoState;
    
    //Set up PB0 and PB1 for digital output.
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DEN)) |= (SI_HI | CLK_HI);
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DIR)) |= (SI_HI | CLK_HI);
    //Initialize the clock line to be LO.
    HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= (CLK_LO);
    
    //Initialize pin PE0 as analog input for input.
    ADC_MultiInit(1);
   
    //Initialize the timer for this module.
    InitializeTimer();
    //Post Timeout event to Camera queue.
    ES_Event thisEvent;
    thisEvent.EventType = ES_TIMEOUT;
    PostCamera(thisEvent);
}

void InitializeTimer(void) {
//Uses WTimer3, timers A and B.
    uint8_t Dummy;
    //Timer for the interrupt generation to pulse clock.
    //Enable clock to timer.
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;
    //Kill a few cycles to let the clock begin.
    Dummy = HWREG(SYSCTL_RCGCGPIO);
    //Disable timer.
    HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TAEN;
    //Set up in 32 bit mode.
    HWREG(WTIMER3_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    //Set up in periodic mode.
    HWREG(WTIMER3_BASE+TIMER_O_TAMR) =
        (HWREG(WTIMER3_BASE+TIMER_O_TAMR)& ~TIMER_TAMR_TAMR_M)|
        TIMER_TAMR_TAMR_PERIOD;
    //Set timeout to CLKTIMER_US.
    HWREG(WTIMER3_BASE+TIMER_O_TAILR) = TICKS_PER_US * CLKTIMER_US/2;
    //Enable a local timeout interrupt.
    HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_TATOIM;
    //Enable timer in NVIC (Interrupt #100).
    HWREG(NVIC_BASE+EN3) |= BIT4HI;
    //MAKE EVERY OTHER INTERRUPT IN THE PROJECT LESS IMPORTANT THAN THIS!
    
    
    //Timer for the interrupt generation to read camera.
    //Enable clock to timer.
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;
    //Kill a few cycles to let the clock begin.
    Dummy = HWREG(SYSCTL_RCGCGPIO);
    //Disable timer.
    HWREG(WTIMER3_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
    //Set up in 32 bit mode.
    HWREG(WTIMER3_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    //Set up in periodic mode.
    HWREG(WTIMER3_BASE+TIMER_O_TBMR) =
        (HWREG(WTIMER3_BASE+TIMER_O_TBMR)& ~TIMER_TBMR_TBMR_M)|
        TIMER_TBMR_TBMR_PERIOD;
    //Set timeout to CLKTIMER_US.
    HWREG(WTIMER3_BASE+TIMER_O_TBILR) = TICKS_PER_US * CAMERA_SAMPLE_US;
    //Enable a local timeout interrupt.
    HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
    //Enable timer in NVIC (Interrupt #101).
    HWREG(NVIC_BASE+EN3) |= BIT5HI;
    //Set the interrupt priority to be lower than the camera CLK.
    HWREG(NVIC_BASE+INTERRUPT_100_TO_103_PRI) |= BIT13HI;
    
    
    //Enable global interrupts.
    __enable_irq();
    //Begin the timer for both timers.
    HWREG(WTIMER3_BASE+TIMER_O_CTL) |= (TIMER_CTL_TAEN |
        TIMER_CTL_TASTALL | TIMER_CTL_TBEN |
        TIMER_CTL_TBSTALL);
}

//PostCamera
bool PostCamera(ES_Event thisEvent) {
//Takes an Event, returns a bool.

    //Post the event to the Camera queue using ES_PostToService.
    //Return the value outputted by ES_PostToService.
    return ES_PostToService(MyPriority, thisEvent);

//End of PostCamera
}

//RunCamera
ES_Event RunCamera(ES_Event thisEvent) {
//Takes an ES_Event, returns an ES_Event
    ES_Event ReturnVal;
    ReturnVal.EventType = ES_NO_EVENT;
    CameraState_t NextState = CurrentState;
    //If there is a reversal, change the angle of the camera and the direction multiplier
    if(thisEvent.EventType == ES_REVERSE_DRIVE) {
        if(ForwardDrive) {
            //Write to the camera servo the reverse servo angle
            WriteCameraServo(REVERSE_SERVO_ANGLE);
            //Change ForwardDrive to false
            ForwardDrive = false;
            //Negate the direction multiplier
            Direction = -Direction;
        } else {
            //Write to the camera servo the reverse servo angle
            WriteCameraServo(FORWARD_SERVO_ANGLE);
            //Change ForwardDrive to false
            ForwardDrive = true;
            //Negate the direction multiplier
            Direction = -Direction;
        }
    }
    //If there's a timeout, enter the state machine
    if(thisEvent.EventType == ES_TIMEOUT) {
        switch(CurrentState) {
            //If in pseudo state
            case InitPseudoState:
                //Move to Reading State
                NextState = Reading;
            break;
            //If reading
            case Reading:
                if(thisEvent.EventParam == WTimerA) {
                    //If BufferInd is 0, start communication by raising SI
                    if(BufferInd == 0 && (CLKState == false)) {
                        HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= (SI_HI);
                    }
                    //Toggle CLK line, since it starts LO, the first toggle will make it HI.
                    HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) ^= (CLK_HI);
                    //Toggle CLKState.
                    CLKState = !CLKState;
                    //If BufferInd is 1, pull SI back down to finish initial communication.
                    //Busy wait loop is used to ensure minimum low time
                    if(BufferInd == 0 && (CLKState == true)) {
                        for(int i = 0; i < 200; i += 1) {
                            SI_Counter = i;
                        }
                        HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= (SI_LO);
                    }
                    //Only record a reading when the CLK is low.
                    if(CLKState == false) {
                        //Get the 128 pixels
                        if(BufferInd < 128) {
                            ADC_MultiRead(AnalogReading);
                            Buffer[BufferInd] = (uint16_t) AnalogReading[0];
                        }
                        BufferInd += 1;
                        //Set the flag of CompletedRead to be false.
                        CompletedRead = false;
                    }
                    if(BufferInd == 129) {
                        //Reset BufferInd
                        BufferInd = 0;
                        //Set the flag of CompletedRead to be true.
                        CompletedRead = true;
                        //Mask the NVIC of the CLK interrupt to not run this function
                        //until the sampling timeout occurs.
                        HWREG(NVIC_BASE+EN3) &= BIT4LO;
                        //Move to Waiting State.
                        NextState = Waiting;
                        //Pull CLK LO.
                        HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= (CLK_LO);
                        //Make CLKState LO.
                        CLKState = false;
                      
                    }
                }
            break;
            //If waiting
            case Waiting:
                //If there is an event timeout from WTimerB (exposure control timer)
                if(thisEvent.EventParam == WTimerB) {
                    //Get the line center
                    float LineCenter = GetLineCenter();
                    //Set next state to be reading
                    NextState = Reading;
                }
            break;
        }
    }
    //Set the current state to the next state
    CurrentState = NextState;
    //Return return value
    return ReturnVal;
}

//ReturnLineCenter
float ReturnLineCenter(void) {
    //Takes nothing, returns a float
    //Return the line center
    return LastLineCenter;
}

//GetLineCenter
float GetLineCenter(void) {
    //Takes nothing, returns a float
    //Only allow this to function when the read is complete,
    //so that there's no attempted calculation in the middle of a read
    if(CompletedRead) {
        //LPF
        uint16_t low_passed[128];
        #ifdef LOWPASS
        //Implement running average of WINDOW_SIZE, special edge cases
        //for when there aren't enough pixels to fill window
        uint16_t average;
        int min, max;
        for(int i = 0; i < 128; i += 1) {
            average = 0;
            min = (i-(WINDOW_SIZE-1)/2);
            max = (i+(WINDOW_SIZE-1)/2);
            if(min < 0) {
                for(int j = i; j <= ((WINDOW_SIZE-1)/2+i); j += 1) {
                    average += Buffer[j];
                }
                average /= ((WINDOW_SIZE-1)/2+1);
            } else if(max > 127) {
                for(int j = i; j >= (i-(WINDOW_SIZE-1)/2); j -= 1) {
                    average += Buffer[j];
                }
                average /= ((WINDOW_SIZE-1)/2+1);
            } else {
                for(int j = (i-(WINDOW_SIZE-1)/2); j <= ((WINDOW_SIZE-1)/2+i); j += 1) {
                    average += Buffer[j];
                }
                average /= WINDOW_SIZE;
            }
            low_passed[i] = average;
            
        }
        #else
        for(int i = 0; i < 128; i += 1) {
            low_passed[i] = Buffer[i];
        }
        #endif
   
        //Normalize and threshold, ignore DEADBAND in normalization
        uint16_t max_value = 0;
        //Find the maximum value
        for (int i = DEADBAND; i < 128-DEADBAND; i += 1) {
            max_value = (Buffer[i] > max_value) ? Buffer[i] : max_value;
        }
        //Find the minimum value
        uint16_t min_value = 4095;
        for (int i = DEADBAND; i < 128-DEADBAND; i += 1) {
            min_value = (Buffer[i] < min_value) ? Buffer[i] : min_value;
        }
      
        //Set the threshold to be 70% of max
        #ifndef LIGHT_THRESHOLD
        uint16_t threshold = (uint16_t) (0.7*4095); // 0.7
        //Scale all the readings from [min, max] to [0, 4095] and enter into boolean array
        for (int i = 0; i < 128; i += 1) {
            //Subtract min value from all values, but make sure that there's no overflow
            if(min_value > low_passed[i]) {
                low_passed[i] = 0;
            } else {
                low_passed[i] -= min_value;
            }
            low_passed[i] = (uint16_t) (4095 * (float) (low_passed[i])/(max_value-min_value));
         
            //Invert low and high
            low_passed[i] = 4095 - low_passed[i];
            //Assign values to thresholded array by comparing to threshold
            thresholded[i] = (low_passed[i] > threshold);
        }
        #else
        uint8_t threshold = (uint8_t) (0.25*4095);
        //Scale all the readings from [min, max] to [0, 4095] and enter into boolean array
        for (int i = 0; i < 128; i += 1) {
            //Invert low and high
            low_passed[i] = 4095 - low_passed[i];
            //Assign values to thresholded array by comparing to threshold
            thresholded[i] = (low_passed[i] > threshold);
        }
        #endif
        
        //Find valid regions, and assign the line to be the smallest region found
        //Set in_region to be false (nothing has been found at the beginning)
        bool in_region = false;
        uint8_t num_regions = 0;
        uint8_t start = 0, end = 0;
        uint8_t smallest_start, smallest_end, smallest_width=255, largest_width = 0;
        uint8_t width, total_width;
        //Loop through all thresholded pixels and find smallest region larger than minimum width
        //outside of deadband
        for (int i = 0; i < 128; i += 1) {
            //Start of a region
            if (!in_region && thresholded[i]) {
                in_region = true;
                start = i;
            }
            //End of a region, record width of the region
            if (in_region && (!thresholded[i] || i == (128-1-DEADBAND))) {
                
                in_region = false;
                end = i;
                //Distinguish between the width of the line, and the total width
                //total width includes deadband area, width does not. This is used
                //for wall and shadow rejection
                total_width = end-start;
                //If the start and end both lie outside of the deadband, then total width = width,
                //if there is a portion in the deadband, distinguish total width and width,
                //if the whole thing is in the deadband, reject the region
                if (start >= DEADBAND && end >= DEADBAND && start <= (128-1-DEADBAND) &&
                    end <= (128-1-DEADBAND)) {
                    width = total_width;
                } else if (start <= DEADBAND && end >= DEADBAND)  {
                    width = end-DEADBAND;
                } else if (start <= (128-1-DEADBAND) && end >= (128-1-DEADBAND)) {
                    width = (128-1-DEADBAND)-start;
                } else {
                    width = 0; //Just consider lines inside the deadbands not lines
                }
                //Find the smallest region
                if (total_width < smallest_width && width > min_allowed_width &&
                    total_width < max_allowed_width) {
                    num_regions++;
                    smallest_start = start;
                    smallest_end = end;
                    smallest_width = total_width;
                }
              
            }
        }
        
        //If there is a line detected, return the middle of the line from -1 to 1
        if (num_regions > 0 && ((max_value - min_value) > MIN_MAX_DIFF_THRESHOLD)) {
            //Set LineFound to be appropriate boolean
            LineFound = true;
            float line_center = (smallest_start + smallest_end)/2.0;
            line_center = -(line_center - 128/2) / (128/2);
            LastLineCenter = line_center*Direction; //Multiply by Direction to account for direction changes
        } else {
            LineFound = false;
        }
    }
    //Return the line center plus a small bias if needed to go off line
    return LastLineCenter + REFERENCE;
}

//ReturnLineFound
bool ReturnLineFound(void) {
    //Takes nothing, returns a bool
    //Return whether a line has been found
    return LineFound;
}

//WTimer3AISR
void WTimer3AISR(void) {
    //Takes nothing, returns nothing
    //Clear the interrupt.
    HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_TATOCINT;
    //Post a Timeout event with event param WTimer3A.
    ES_Event thisEvent;
    thisEvent.EventType = ES_TIMEOUT;
    thisEvent.EventParam = WTimerA;
    PostCamera(thisEvent);
}

//WTimer3BISR
void WTimer3BISR(void) {
    //Takes nothing, returns nothing
    //Clear the interrupt.
    HWREG(WTIMER3_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;
    //Unmask the NVIC for the camera CLK interrupt (interrupt #100)
    HWREG(NVIC_BASE+EN3) |= BIT4HI;
    //Post a Timeout event.
    ES_Event thisEvent;
    thisEvent.EventType = ES_TIMEOUT;
    thisEvent.EventParam = WTimerB;
    PostCamera(thisEvent);
}
