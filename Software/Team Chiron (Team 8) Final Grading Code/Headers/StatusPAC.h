/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef StatusPAC_H
#define StatusPAC_H

// State definitions for use with the query function
typedef enum { WAITING_PAC, SENDING } StatusPACState_t ;

// Public Function Prototypes

ES_Event RunStatusPAC( ES_Event CurrentEvent );
void StartStatusPAC ( ES_Event CurrentEvent );
bool PostStatusPAC( ES_Event ThisEvent );
bool InitStatusPAC ( uint8_t Priority );

//#define DEBUG_PAC

#endif /*StatusPAC_H */

