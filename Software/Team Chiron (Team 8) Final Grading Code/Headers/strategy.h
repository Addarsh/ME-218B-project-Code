/****************************************************************************
 
  Header file for template Flat Sate Machine 
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef Strategy_H
#define Strategy_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitPState, WaitingForCommand, Between_Capture, WaitingUntilRequestDelayIsOver} MyState_t ;


// Public Function Prototypes

bool InitStrategySM ( uint8_t Priority );
bool PostStrategySM( ES_Event ThisEvent );
ES_Event RunStrategySM( ES_Event ThisEvent );
MyState_t QueryStrategySM ( void );
uint8_t getMyColor(void);


#endif /* FSMTemplate_H */

