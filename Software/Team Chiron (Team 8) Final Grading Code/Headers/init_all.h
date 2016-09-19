
#ifndef INIT_ALL_H
#define INIT_ALL_H

#include "Direction.h"
void initialize_all(void);
//uint32_t getPulsePeriodA(void);
//uint32_t getPulsePeriodB(void);
uint32_t getPeriodFromHall(void);
void disableHall(void);
void enableHall(void);
void disableIR(void);
void enableIR(void);
uint32_t getIRFrequency(void);
uint8_t getPartySwith(void);
Direction getCurrentDirection(void);
void setTriggerHI(void);
void setTriggerLO(void);
void setDisplayLED(void);
void turnOffLED(void);




#endif

