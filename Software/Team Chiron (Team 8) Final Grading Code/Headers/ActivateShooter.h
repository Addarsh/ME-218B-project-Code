/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef ActivateShooter_H
#define ActivateShooter_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { WaitingToShoot, AlignWithBeacons, ActivateMotor,GoBack} ShooterState_t ;


// Public Function Prototypes

ES_Event RunShooter( ES_Event CurrentEvent );
void StartShooter ( ES_Event CurrentEvent );
ShooterState_t QueryShooter ( void );

#endif /*SHMTemplate_H */

