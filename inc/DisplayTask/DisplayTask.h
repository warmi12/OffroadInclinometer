#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

typedef enum 
{
    DISPLAY_INIT,
    DISPLAY_LOOP,
    DISPLAY_ERROR,
} DisplayTaskState_t;

void vDisplayTask( void *pvParameters );
void vChagneScreen( void );
void vSetCalibrationValue( float rollCalibVal, float pitchCalibVal );

#endif