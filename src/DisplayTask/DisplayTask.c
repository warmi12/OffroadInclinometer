#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lvgl.h"

#include "DisplayTask.h"
#include "LvglProcess.h"

static DisplayTaskState_t DisplayTaskState;

static void vInit( void );
static void vLoop( void );

static void vInit( void )
{
    vLvglInit();
    vWidgetsInit();

    DisplayTaskState = DISPLAY_LOOP;
}

static void vLoop( void )
{
    int time = 0;
    while(1)
    {
        lv_task_handler();
        vRefreshIMUDataHandler();
        vTaskDelay( 5 / portTICK_PERIOD_MS );
        
        // printf("%d, %d\n", get_core_num(), time);
        // time++;
        // vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void vDisplayTask( void *pvParameters ) 
{
    (void) pvParameters;

    while (1)
    {       
        switch (DisplayTaskState)
        {
        case DISPLAY_INIT:
            vInit();
            break;
        case DISPLAY_LOOP:
            vLoop();
            break;
        case DISPLAY_ERROR:
            break;
        default:
            break;
        }
    }

	// int time = 0;
	// while(1)
	// {
	// 	printf("%d, %d\n", get_core_num(), time);
	// 	time++;
	// 	vTaskDelay(1000 / portTICK_PERIOD_MS);
	// }
}