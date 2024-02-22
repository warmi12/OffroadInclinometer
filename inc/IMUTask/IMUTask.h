#ifndef IMU_TASK_H
#define IMU_TASK_H

#include "Fusion.h"

#define SAMPLE_RATE 1000U
#define WAIT_TIMEOUT 1000U
#define SEC 1
#define CALIB_TIME_MS (5 * SEC * SAMPLE_RATE)

typedef enum
{
    IMU_INIT,
    IMU_CALIB,
    IMU_AHRS,
    IMU_ERROR    
} IMUTaskState_t;

typedef struct IMUData
{
    float acc[3];
    float gyro[3];
    unsigned int timestamp;
    unsigned int previousTimestamp;
    float deltaTime;
} IMUData_t;

typedef struct IMUCalibrationData
{
    float acc[3];
    float gyro[3];
    unsigned int sampleCounter;
} IMUCalibrationData_t;

void vIMUCallback( void );
void vIMUTask( void *pvParameters );
FusionEuler *pGetEulerAngles( void );

#endif