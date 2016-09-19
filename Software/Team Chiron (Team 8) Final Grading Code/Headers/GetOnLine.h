#ifndef GETONLINE_H
#define GETONLINE_H

#include <stdint.h>

//Public functions
void StartGetOnLine(ES_Event thisEvent);
bool InitializeGetOnLine(uint8_t priority);
bool PostGetOnLine(ES_Event thisEvent);
ES_Event RunGetOnLine(ES_Event thisEvent);

//States
typedef enum {WaitingToStart, AlignWithBeacon, FullSpeedForward, LineUp} GetOnLineState_t;

#endif

