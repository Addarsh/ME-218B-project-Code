/****************************************************************************
 
  Header file for template Flat Sate Machine 
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef ComputeDirection_H
#define ComputeDirection_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitPState,Waiting} ComputeDirectionState_t ;


// Public Function Prototypes

bool InitComputeDirectionSM ( uint8_t Priority );
bool PostComputeDirectionSM( ES_Event ThisEvent );
ES_Event RunComputeDirectionSM( ES_Event ThisEvent );
ComputeDirectionState_t QueryComputeDirectionSM ( void );


#endif /* FSMTemplate_H */

