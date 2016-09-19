#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>

//States
typedef enum {InitPseudoState, Waiting, Reading} CameraState_t;

//Public functions
void InitializeCamera(uint8_t priority);
bool PostCamera(ES_Event thisEvent);
void InitializeTimer(void);
ES_Event RunCamera(ES_Event thisEvent);
float GetLineCenter(void);
float ReturnLineCenter(void);
void WTimer3AISR(void);
bool ReturnLineFound(void);


#endif
