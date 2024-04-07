#include "FreeRTOS.h"
#include "semphr.h"

#include "stdio.h"
#include "pico/stdlib.h"

#include "IMUTask.h"
#include "DisplayTask.h"
#include "QMI8658.h"
#include "string.h"
#define MAX(a,b) ((a > b) ? (a) : (b))
#define MIN(a,b) ((a < b) ? (a) : (b))

static IMUTaskState_t IMUTaskState;
static IMUData_t IMUData;
static IMUCalibrationData_t IMUCalibrationData;

static SemaphoreHandle_t xIMUCallbackSemaphore = NULL;
SemaphoreHandle_t xDisplaySemaphore = NULL;

static FusionAhrs ahrs;
static FusionOffset offset;
static FusionAhrsFlags flags;
static const FusionAhrsSettings settings = {
    .convention = FusionConventionEnu,
    .gain = 0.5f,
    .gyroscopeRange = 512.0f, /* replace this with actual gyroscope range in degrees/s */
    .accelerationRejection = 10.0f,
    .magneticRejection = 10.0f,
    .recoveryTriggerPeriod = 1 * SAMPLE_RATE, /* 5 seconds */
};

typedef union {
    float float_val;
    uint8_t bytes[4];
} unia;

uint8_t buffer[16];

unia float_to_bytes;

static FusionVector gyroscope;
static FusionVector accelerometer;
static FusionEuler euler;

static unsigned int stationarySamplesCounter = 0;
static float stationaryMin[2]={1000};
static float stationaryMax[2]={-1000};

static void vInit( void );
static void vCalibration( void );
static void vAHRS( void );
static bool bIsStationary( void );

static void vInit( void )
{
    FusionOffsetInitialise(&offset, SAMPLE_RATE);
    FusionAhrsInitialise(&ahrs);
    FusionAhrsSetSettings(&ahrs, &settings);

    // xIMUCallbackSemaphore = xSemaphoreCreateBinary();
    // xDisplaySemaphore = xSemaphoreCreateMutex();

    IMUTaskState = IMU_CALIB;
}

static void vCalibration( void )
{
   // stationarySamplesCounter = 0;
  //  while(1)
   // {
       // if( xSemaphoreTake( xIMUCallbackSemaphore, WAIT_TIMEOUT / portTICK_PERIOD_MS ) == pdTRUE )
      //  {
            QMI8658_read_xyz(IMUData.acc, IMUData.gyro, &IMUData.timestamp);

            // IMUCalibrationData.acc[0] += IMUData.acc[0];
            // IMUCalibrationData.acc[1] += IMUData.acc[1];
            // IMUCalibrationData.acc[2] += IMUData.acc[2];

            IMUCalibrationData.gyro[0] += IMUData.gyro[0];
            IMUCalibrationData.gyro[1] += IMUData.gyro[1];
            IMUCalibrationData.gyro[2] += IMUData.gyro[2];
            
            gyroscope.array[0] = IMUData.gyro[0];
            gyroscope.array[1] = IMUData.gyro[1];
            gyroscope.array[2] = IMUData.gyro[2];

            accelerometer.array[0] = IMUData.acc[0];
            accelerometer.array[1] = IMUData.acc[1];
            accelerometer.array[2] = IMUData.acc[2];

            FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMUData.deltaTime);

            if (IMUCalibrationData.sampleCounter >= CALIB_TIME_MS) 
            {
                IMUCalibrationData.acc[0] /= CALIB_TIME_MS;
                IMUCalibrationData.acc[1] /= CALIB_TIME_MS;
                IMUCalibrationData.acc[2] /= CALIB_TIME_MS;

                IMUCalibrationData.gyro[0] /= CALIB_TIME_MS;
                IMUCalibrationData.gyro[1] /= CALIB_TIME_MS;
                IMUCalibrationData.gyro[2] /= CALIB_TIME_MS;
                //break;

                IMUTaskState = IMU_AHRS;
            }
            else
            {
                IMUCalibrationData.sampleCounter++;
            }
        //}
    //}

    // while(1)
    // {
    //     if ( xSemaphoreTake( xIMUCallbackSemaphore, WAIT_TIMEOUT / portTICK_PERIOD_MS ) == pdTRUE )
    //     {
    //         QMI8658_read_xyz(IMUData.acc, IMUData.gyro, &IMUData.timestamp);

    //         IMUData.deltaTime = (float) (IMUData.timestamp - IMUData.previousTimestamp) / 2000.0f;
    //         IMUData.previousTimestamp = IMUData.timestamp;

    //         IMUData.gyro[0] -= IMUCalibrationData.gyro[0];
    //         IMUData.gyro[1] -= IMUCalibrationData.gyro[1];
    //         IMUData.gyro[2] -= IMUCalibrationData.gyro[2];

    //         gyroscope.array[0] = IMUData.gyro[0];
    //         gyroscope.array[1] = IMUData.gyro[1];
    //         gyroscope.array[2] = IMUData.gyro[2];

    //        // IMUData.acc[0] *=-1.0f;

    //         accelerometer.array[0] = IMUData.acc[0];
    //         accelerometer.array[1] = IMUData.acc[1];
    //         accelerometer.array[2] = IMUData.acc[2];

    //         FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMUData.deltaTime);
            
    //         if( xSemaphoreTake( xDisplaySemaphore, 1) == pdTRUE )
    //         {
    //             euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
    //             xSemaphoreGive( xDisplaySemaphore );
    //         }

    //         //printf("euler %0.1f %0.1f\n", euler.angle.roll, euler.angle.pitch);

    //         if ( bIsStationary() )
    //         {
    //             vChagneScreen();
    //             vSetCalibrationValue( stationaryMin[0], stationaryMin[1] );
    //             break;
    //         }
    //     }
    // }
    
    //    DEBUG!
    //    TODO: DEBUG MAKRO!
    //printf("CALIB ACC: %0.1f %0.1f %0.1f\n 
    //        CALIB GYR: %0.1f %0.1f %0.1f\n", 
    //        IMUCalibrationData.acc[0], IMUCalibrationData.acc[1], IMUCalibrationData.acc[2], 
    //        IMUCalibrationData.gyro[0], IMUCalibrationData.gyro[1], IMUCalibrationData.gyro[2] 
    //      );

   // IMUTaskState = IMU_AHRS;
}

//DEBUG
unsigned int time = 0;

static void vAHRS( void )
 {
//     while(1)
//     {
//         if ( xSemaphoreTake( xIMUCallbackSemaphore, WAIT_TIMEOUT / portTICK_PERIOD_MS ) == pdTRUE )
//         {
            //unsigned long start_time = time_us_64();

            QMI8658_read_xyz(IMUData.acc, IMUData.gyro, &IMUData.timestamp);

            // unsigned long end_time = time_us_64(); 
            // unsigned long execution_time = (end_time - start_time);

            // if (IMUData.timestamp - time > 500U){
            //     time = IMUData.timestamp;
            //     printf("TIME: %ld\n", execution_time);
            // }

            IMUData.deltaTime = (float) (IMUData.timestamp - IMUData.previousTimestamp) / 2000.0f;
            IMUData.previousTimestamp = IMUData.timestamp;

            IMUData.gyro[0] -= IMUCalibrationData.gyro[0];
            IMUData.gyro[1] -= IMUCalibrationData.gyro[1];
            IMUData.gyro[2] -= IMUCalibrationData.gyro[2];

            gyroscope.array[0] = IMUData.gyro[0];
            gyroscope.array[1] = IMUData.gyro[1];
            gyroscope.array[2] = IMUData.gyro[2];

           // IMUData.acc[0] *=-1.0f;

            accelerometer.array[0] = IMUData.acc[0];
            accelerometer.array[1] = IMUData.acc[1];
            accelerometer.array[2] = IMUData.acc[2];

            FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, IMUData.deltaTime);
            
            // if( xSemaphoreTake( xDisplaySemaphore, 1) == pdTRUE )
            // {
            //     euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
            //     //euler.angle.roll -= stationaryMin[0];
            //     // euler.angle.pitch -= stationaryMin[1];
            //     xSemaphoreGive( xDisplaySemaphore );
            // }


            

    //    }

            if (IMUData.timestamp - time > (2 * 100U))
            {
                int buffer_counter = 0;

                time = IMUData.timestamp;

                euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));

                float_to_bytes.float_val=euler.angle.roll;
                buffer[buffer_counter] = 0xAA;
                buffer_counter++;
                buffer[buffer_counter] = 0xAA;
                buffer_counter++;

                buffer[buffer_counter] = float_to_bytes.bytes[0];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[1];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[2];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[3];
                buffer_counter++;

                float_to_bytes.float_val=euler.angle.pitch;

                buffer[buffer_counter] = float_to_bytes.bytes[0];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[1];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[2];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[3];
                buffer_counter++;

                float_to_bytes.float_val=euler.angle.yaw;

                buffer[buffer_counter] = float_to_bytes.bytes[0];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[1];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[2];
                buffer_counter++;
                buffer[buffer_counter] = float_to_bytes.bytes[3];
                buffer_counter++;

                int counter = 0;
                uint8_t crc = 0;
                while (counter < buffer_counter)
                {
                    crc ^=  buffer[counter];
                    counter++;
                }

                
                buffer[buffer_counter] = 0x00;
                buffer_counter++;
                buffer[buffer_counter] = (crc & 0xFF);
                buffer_counter++;

                counter = 0;

                while (counter < buffer_counter)
                {
                    putchar_raw(buffer[counter]);
                    counter++;
                }

                memset(&buffer,0,sizeof(buffer));
            }

        
        //    DEBUG
        //    TODO: DEBUG MAKRO!
        // if (IMUData.timestamp - time > 500U)
        // {
        //     time = IMUData.timestamp;
        //     flags = FusionAhrsGetFlags(&ahrs);
        //     uint core_num = get_core_num();
        //     printf("Roll %0.1f, Pitch %0.1f, Yaw %0.1f, Core num: %d, INIT FLAG: %d\n", 
        //     euler.angle.roll, euler.angle.pitch, euler.angle.yaw, core_num, flags.initialising
        //     );
        // }
        
   // }
}

static bool bIsStationary( void )
{
    if ( stationarySamplesCounter < 6000U )
    {
        stationaryMin[0]=MIN(stationaryMin[0],(euler.angle.roll));
        stationaryMax[0]=MAX(stationaryMax[0],(euler.angle.roll));

        stationaryMin[1]=MIN(stationaryMin[1],(euler.angle.pitch));
        stationaryMax[1]=MAX(stationaryMax[1],(euler.angle.pitch));
        
        stationarySamplesCounter++;

        return false;
    }
    else
    {
         stationarySamplesCounter = 0;

         //printf("VALUES: %0.1f, %0.1f, %0.1f, %0.1f\n", stationaryMax[0], stationaryMin[0], stationaryMax[1], stationaryMin[1]);

         if ( (stationaryMax[0] - stationaryMin[0] < 0.3f) && (stationaryMax[1] - stationaryMin[1] < 0.3f) /*&& euler.angle.roll < 170.0f*/)
         {
            return true;
         }
         else
         {
            stationaryMin[0] = 1000;
            stationaryMin[1] = 1000;
            
            stationaryMax[0] = -1000;
            stationaryMax[1] = -1000;
            return false;
         }
    }
}

void vIMUCallback( void )
{
    if (gpio_get_irq_event_mask(24) & GPIO_IRQ_EDGE_RISE) 
    {
       gpio_acknowledge_irq(24, GPIO_IRQ_EDGE_RISE);
       
    //    if (IMUTaskState != IMU_INIT && IMUTaskState != IMU_ERROR)
    //    {
    //     xSemaphoreGiveFromISR( xIMUCallbackSemaphore, pdFALSE);
    //    }
        if  ( IMUTaskState == IMU_CALIB )
        {
            vCalibration();
        } 
        else if (IMUTaskState == IMU_AHRS)
        {
            vAHRS();
        }
    }
} 

FusionEuler *pGetEulerAngles( void )
{
    return &euler;
}

void vIMUTask( void *pvParameters )
{
    (void) pvParameters;

    while(1)
    {
        switch (IMUTaskState)
        {
        case IMU_INIT:
            vInit();
            break;
        case IMU_CALIB:
            //vCalibration();
            break;
        case IMU_AHRS:
            //vAHRS();
            break;
        case IMU_ERROR:
            vTaskDelay( 500 / portTICK_PERIOD_MS );
            break;
        default:
            break;
        }
    }
}