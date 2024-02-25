#ifndef LVGL_PROCESS_H
#define LVGL_PROCESS_H

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "semphr.h"

extern SemaphoreHandle_t xDisplaySemaphore;

void vLvglInit( void );
void vWidgetsInit( void );
void vRefreshIMUDataHandler();
void vChangeToMainScreen();
void vSetCalibVal( float rollVal, float pitchVal );

#endif
