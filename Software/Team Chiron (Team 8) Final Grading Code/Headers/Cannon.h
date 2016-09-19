#ifndef CANNON_H

#define CANNON_H

//Public Functions
bool InitCannon(uint8_t priority);
bool PostCannon(ES_Event thisEvent);
ES_Event RunCannon(ES_Event thisEvent);

//Private Functions
static void Shoot(void);
static void Reload(void);
static void Arm(void);

//States
typedef enum {CannonIdle, Arming, Shooting, Reloading} Cannon_State_t;

#endif
