#ifndef FollowLine_H
#define FollowLine_H


// typedefs for the states
// State definitions for use with the query function
typedef enum {FollowWaiting, Following, Reversing, Rotating} FollowLine_t ;


// Public Function Prototypes

ES_Event RunFollowLine(ES_Event CurrentEvent);
void StartFollowLine(ES_Event CurrentEvent);
FollowLine_t QueryFollowLine(void);

//Private Function Prototypes
static ES_Event DuringStateRotating(ES_Event Event);
static ES_Event DuringStateReversing(ES_Event Event);


#endif /*SHMTemplate_H */
