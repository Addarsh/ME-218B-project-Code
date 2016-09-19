/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef CaptureStation_H
#define CaptureStation_H

// State definitions for use with the query function
typedef enum { WAITING_CAPTURE, WAIT_FOR_REQUEST_PAC, SEND_QUERY_PAC, WAIT_FOR_QUERY_RESPONSE } Capture_t ;

// Public Function Prototypes

ES_Event RunCaptureStation( ES_Event CurrentEvent );
void StartCaptureStation ( ES_Event CurrentEvent );
Capture_t QueryCaptureSM ( void );

#endif /*TopHSMTemplate_H */

