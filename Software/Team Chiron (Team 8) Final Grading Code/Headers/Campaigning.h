/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts

 ****************************************************************************/

#ifndef Campaigning_H
#define Campaigning_H

// State definitions for use with the query function
typedef enum { WaitingToStartCampaign, FollowLine, CaptureStation, ActivateShooter } CampaignState_t ;

// Public Function Prototypes

ES_Event RunCampaigningSM( ES_Event CurrentEvent );
void StartCampaigningSM ( ES_Event CurrentEvent );
bool PostCampaigningSM( ES_Event ThisEvent );
bool InitCampaigningSM ( uint8_t Priority );

#endif /*TopHSMTemplate_H */

