#include "FreeRTOS.h"
#include "semphr.h"

#include "stdio.h"
#include "pico/stdlib.h"

#include "IMUTask.h"
#include "Fusion.h"
#include "QMI8658.h"

IMUTaskState_t IMUTaskState;
IMUData_t IMUData;
IMUCalibrationData_t IMUCalibrationData;

SemaphoreHandle_t xIMUSemaphore = NULL;

FusionAhrs ahrs;
FusionOffset offset;
const FusionAhrsSettings settings = {
    .convention = FusionConventionEnu,
    .gain = 2.0f,
    .gyroscopeRange = 2000.0f, /* replace this with actual gyroscope range in degrees/s */
    .accelerationRejection = 10.0f,
    .magneticRejection = 10.0f,
    .recoveryTriggerPeriod = 5 * SAMPLE_RATE, /* 5 seconds */
};

FusionVector gyroscope;
FusionVector accelerometer;
FusionEuler euler;

static void vInit(void);
static void vCalibration(void);
static void vAHRS(void);

static void vInit(void)
{
    FusionOffsetInitialise(&offset, SAMPLE_RATE);
    FusionAhrsInitialise(&ahrs);
    FusionAhrsSetSettings(&ahrs, &settings);

    xIMUSemaphore = xSemaphoreCreateBinary();

    IMUTaskState = CALIB;
}

static void vCalibration(void)
{
    while(1)
    {
        if( xSemaphoreTake( xIMUSemaphore, WAIT_TIMEOUT / portTICK_PERIOD_MS ) == pdTRUE )
        {
            QMI8658_read_xyz(IMUData.acc, IMUData.gyro, &IMUData.timestamp);

            IMUCalibrationData.acc[0] += IMUData.acc[0];
            IMUCalibrationData.acc[1] += IMUData.acc[1];
            IMUCalibrationData.acc[2] += IMUData.acc[2];

            IMUCalibrationData.gyro[0] += IMUData.gyro[0];
            IMUCalibrationData.gyro[1] += IMUData.gyro[1];
            IMUCalibrationData.gyro[2] += IMUData.gyro[2];

            if (IMUCalibrationData.sampleCounter >= CALIB_TIME_MS) 
            {
                IMUCalibrationData.acc[0] /= CALIB_TIME_MS;
                IMUCalibrationData.acc[1] /= CALIB_TIME_MS;
                IMUCalibrationData.acc[2] /= CALIB_TIME_MS;

                IMUCalibrationData.gyro[0] /= CALIB_TIME_MS;
                IMUCalibrationData.gyro[1] /= CALIB_TIME_MS;
                IMUCalibrationData.gyro[2] /= CALIB_TIME_MS;
                break;
            }
            else
            {
                IMUCalibrationData.sampleCounter++;
            }
        }
    }
    
    /* 
        DEBUG!
        TODO: DEBUG MAKRO!
    printf("CALIB ACC: %0.1f %0.1f %0.1f\n 
            CALIB GYR: %0.1f %0.1f %0.1f\n", 
            IMUCalibrationData.acc[0], IMUCalibrationData.acc[1], IMUCalibrationData.acc[2], 
            IMUCalibrationData.gyro[0], IMUCalibrationData.gyro[1], IMUCalibrationData.gyro[2] 
          );
    */
    IMUTaskState = AHRS;
}

void vAHRS(void)
{
    while(1)
    {
        if ( xSemaphoreTake( xIMUSemaphore, WAIT_TIMEOUT / portTICK_PERIOD_MS ) == pdTRUE )
        {
            QMI8658_read_xyz(IMUData.acc, IMUData.gyro, &IMUData.timestamp);

            IMUData.deltaTime = (float) (IMUData.timestamp - IMUData.previousTimestamp) / 1000.0f;
            IMUData.previousTimestamp = IMUData.timestamp;

            gyroscope.array[0] = IMUData.gyro[0];
            gyroscope.array[1] = IMUData.gyro[1];
            gyroscope.array[2] = IMUData.gyro[2];

            accelerometer.array[0] = IMUData.acc[0];
            accelerometer.array[1] = IMUData.acc[1];
            accelerometer.array[2] = IMUData.acc[2];

            FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMUData.deltaTime);
            euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
        }

        /* 
            DEBUG
            TODO: DEBUG MAKRO!
        if (IMUData.timestamp - time > 500U)
        {
            time = IMUData.timestamp;
            uint core_num = get_core_num();
            printf("Roll %0.1f, Pitch %0.1f, Yaw %0.1f, Core num: %d\n", 
            euler.angle.roll, euler.angle.pitch, euler.angle.yaw, core_num
            );
        }
        */
    }
}

void vIMUCallback( void )
{
    if (gpio_get_irq_event_mask(24) & GPIO_IRQ_EDGE_RISE) 
    {
       gpio_acknowledge_irq(24, GPIO_IRQ_EDGE_RISE);
       
       if (IMUTaskState != INIT && IMUTaskState != ERROR)
       {
        xSemaphoreGiveFromISR( xIMUSemaphore, pdFALSE);
       }
    }
} 

void vIMUTask( void *pvParameters )
{
    (void) pvParameters;

    while(1)
    {
        switch (IMUTaskState)
        {
        case INIT:
            vInit();
            break;
        case CALIB:
            vCalibration();
            break;
        case AHRS:
            vAHRS();
            break;
        case ERROR:
            break;
        default:
            break;
        }
    }
}