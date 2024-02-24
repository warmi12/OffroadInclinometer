#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "Fusion.h"
#include "DEV_Config.h"
#include "LCD_1in28.h"
#include "QMI8658.h"

#include "IMUTask.h"
#include "DisplayTask.h"

#define IMU_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define DISPLAY_TASK_PRIORITY   ( tskIDLE_PRIORITY + 1 )

static void prvSetupHardware( void );

int main( void )
{
	    prvSetupHardware();

	xTaskCreate( vIMUTask,				/* The function that implements the task. */
				"IMUTask", 							/* The text name assigned to the task - for debug only as it is not used by the kernel. */
				configMINIMAL_STACK_SIZE * 4, 			/* The size of the stack to allocate to the task. */
				NULL, 								/* The parameter passed to the task - not used in this case. */
				IMU_TASK_PRIORITY, 	/* The priority assigned to the task. */
				NULL );								/* The task handle is not required, so NULL is passed. */

	xTaskCreate( vDisplayTask,				/* The function that implements the task. */
				"DisplayTask", 							/* The text name assigned to the task - for debug only as it is not used by the kernel. */
				configMINIMAL_STACK_SIZE * 4, 			/* The size of the stack to allocate to the task. */
				NULL, 								/* The parameter passed to the task - not used in this case. */
				DISPLAY_TASK_PRIORITY, 	/* The priority assigned to the task. */
				NULL );								/* The task handle is not required, so NULL is passed. */

	vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
    DEV_Module_Init();

	LCD_1IN28_Init(HORIZONTAL);   

    QMI8658_init();
}