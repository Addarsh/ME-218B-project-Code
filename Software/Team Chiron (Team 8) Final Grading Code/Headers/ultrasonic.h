/****************************************************************************

  Header file for Ultrasonic
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ultrasonic_H
#define ultrasonic_H

#include "ES_Configure.h"
#include "ES_Types.h"

typedef enum {Firing, WaitForEcho, FindDistance} UltraState_t ;

// Public Function Prototypes

bool InitUltra ( uint8_t Priority );
bool PostUltra( ES_Event ThisEvent );
ES_Event RunUltra( ES_Event ThisEvent );
float querydistance(void);

#endif /*ultrasonic_H*/
