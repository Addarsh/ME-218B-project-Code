/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef SendBytes_H
#define SendBytes_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { WAITING_SENDBYTES, SENDING_SENDBYTES, WAITING4EOT, WAITING4TIMEOUT } SendBytesState_t ;


// Public Function Prototypes

ES_Event RunSendBytesStatus( ES_Event CurrentEvent );
void StartSendBytesStatus ( ES_Event CurrentEvent );
SendBytesState_t QuerySendBytesStatus ( void );

#endif /*SendBytesStatus_H */

