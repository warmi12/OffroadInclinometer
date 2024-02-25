#include "DEV_Config.h"
#include "LCD_1in28.h"
#include "lvgl.h"

#include "LvglProcess.h"
#include "IMUTask.h"
#include "IMGs.h"

#define UNUSED(x) ((void)(x))
#define DISP_HOR_RES 240
#define DISP_VER_RES 240

static bool RefreshIMUDataHandlerFlag = false;

static FusionEuler actualEulerAngles;
static float rollCalibVal;
static float pitchCalibVal;

//after
static lv_obj_t *tileView;

static lv_obj_t *tile1;
static lv_obj_t *txtTile1;
static lv_obj_t *loader;

static lv_obj_t *tile2;
static lv_obj_t *txtRollTile2;
static lv_obj_t *txtPitchTile2;
static lv_obj_t *indicatorLeft;
static lv_obj_t *indicatorRight;
static lv_obj_t *backgroundImg;
static lv_obj_t *frontCarImg;
static lv_obj_t *sideCarImg;

static struct repeating_timer xLvglTimer;
static struct repeating_timer xIMUTimer;

static lv_disp_draw_buf_t xLvglDispBuf;
static lv_color_t xLvglDispBuf0[ DISP_HOR_RES * (DISP_VER_RES/2) ];
static lv_color_t xLvglDispBuf1[ DISP_HOR_RES * (DISP_VER_RES/2) ];
static lv_disp_drv_t xLvglDispDrv;

static bool bLvglTimerCallback( struct repeating_timer *t );
static bool bRefreshIMUDataTimerCallback( struct repeating_timer *t );
static void vSetRefreshIMUDataHandlerFlag();
static void vUnsetRefreshIMUDataHandlerFlag();

static void vDispFlushCb( lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p );
static void vDmaHandler( void );

void vLvglInit( void )
{
    add_repeating_timer_ms( 5, bLvglTimerCallback, NULL, &xLvglTimer );
    add_repeating_timer_ms( 500, bRefreshIMUDataTimerCallback, NULL, &xIMUTimer );

    lv_init();

    lv_disp_draw_buf_init( &xLvglDispBuf, xLvglDispBuf0, xLvglDispBuf1, DISP_HOR_RES * DISP_VER_RES / 2 ); 
    lv_disp_drv_init( &xLvglDispDrv );  
    xLvglDispDrv.flush_cb = vDispFlushCb;
    xLvglDispDrv.draw_buf = &xLvglDispBuf;        
    xLvglDispDrv.hor_res = DISP_HOR_RES;
    xLvglDispDrv.ver_res = DISP_VER_RES;
    lv_disp_drv_register(&xLvglDispDrv); 

    dma_channel_set_irq0_enabled( dma_tx, true );
    irq_set_exclusive_handler( DMA_IRQ_0, vDmaHandler );
    irq_set_enabled( DMA_IRQ_0, true );
}

static void set_angle(void * obj, int32_t v)
{
    lv_arc_set_value(obj, v);
}

void vWidgetsInit( void )
{
    //Calibration screen
    tileView = lv_tileview_create(lv_scr_act());
    lv_obj_set_style_bg_color(tileView, lv_color_hex(0x003a57), LV_PART_MAIN);

    tile1 = lv_tileview_add_tile(tileView, 0, 0, LV_DIR_NONE);
    
    txtTile1 = lv_label_create(tile1);
    lv_label_set_text(txtTile1, "CALIBRATION");
    lv_obj_center(txtTile1);
    lv_obj_set_style_text_color(txtTile1, lv_color_hex(0xffffff), LV_PART_MAIN);
    
    loader = lv_arc_create(tile1);
    lv_arc_set_rotation(loader, 270);
    lv_arc_set_bg_angles(loader, 0, 360);
    lv_obj_set_size(loader, 60, 60);
    lv_obj_remove_style(loader, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(loader, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_align(loader, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_anim_t loadingAnim;
    lv_anim_init(&loadingAnim);
    lv_anim_set_var(&loadingAnim, loader);
    lv_anim_set_exec_cb(&loadingAnim, set_angle);
    lv_anim_set_time(&loadingAnim, 1000);
    lv_anim_set_repeat_count(&loadingAnim, LV_ANIM_REPEAT_INFINITE);    /*Just for the demo*/
    lv_anim_set_repeat_delay(&loadingAnim, 500);
    lv_anim_set_values(&loadingAnim, 0, 100);
    lv_anim_start(&loadingAnim);
    
    //Main Screen
    tile2 = lv_tileview_add_tile(tileView, 0, 1, LV_DIR_NONE);

    /* IMAGE */
    LV_IMG_DECLARE(background);
    LV_IMG_DECLARE(sideCar);
    LV_IMG_DECLARE(frontCar);

    backgroundImg = lv_img_create(tile2);
    lv_img_set_src(backgroundImg, &background);
    lv_obj_set_pos(backgroundImg, 0, 0);

    /* INDICATORS */
    indicatorLeft = lv_arc_create(tile2);
    
    lv_obj_set_size(indicatorLeft, 170, 170);
    lv_obj_remove_style(indicatorLeft, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(indicatorLeft, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_set_pos(indicatorLeft, 30, 35);

    lv_arc_set_rotation(indicatorLeft, 130);
    lv_arc_set_bg_angles(indicatorLeft, 0, 100);
    lv_obj_set_style_arc_width(indicatorLeft, 10, 0);
    lv_obj_set_style_arc_width(indicatorLeft, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0x003a57), 0);
    lv_arc_set_value(indicatorLeft, 0);
    lv_arc_set_mode(indicatorLeft, LV_ARC_MODE_SYMMETRICAL);
    
    lv_anim_t indicatorLeftAnim;
    lv_anim_init(&indicatorLeftAnim);
    lv_anim_set_var(&indicatorLeftAnim, indicatorLeft);
    lv_anim_set_values(&indicatorLeftAnim, 0, 100);
    lv_anim_start(&indicatorLeftAnim);

    indicatorRight = lv_arc_create(tile2);
    
    lv_obj_set_size(indicatorRight, 170, 170);
    lv_obj_remove_style(indicatorRight, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(indicatorRight, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_set_pos(indicatorRight, 40, 35);

    lv_arc_set_rotation(indicatorRight, 310);
    lv_arc_set_bg_angles(indicatorRight, 0, 100);
    lv_obj_set_style_arc_width(indicatorRight, 10, 0);
    lv_obj_set_style_arc_width(indicatorRight, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0x003a57), 0);
    lv_arc_set_value(indicatorRight, 0);
    lv_arc_set_mode(indicatorRight, LV_ARC_MODE_SYMMETRICAL);
    
    lv_anim_t indicatorRightAnim;
    lv_anim_init(&indicatorRightAnim);
    lv_anim_set_var(&indicatorRightAnim, indicatorRight);
    lv_anim_set_values(&indicatorRightAnim, 0, 100);
    lv_anim_start(&indicatorRightAnim);

    txtRollTile2 = lv_label_create(backgroundImg);
    lv_label_set_text(txtRollTile2, "50");
    lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(txtRollTile2, LV_ALIGN_TOP_MID, 0, 0);

    txtPitchTile2 = lv_label_create(backgroundImg);
    lv_label_set_text(txtPitchTile2, "50");
    lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(txtPitchTile2, LV_ALIGN_BOTTOM_MID, 0, 0);

    frontCarImg = lv_img_create(backgroundImg);
    lv_img_set_src(frontCarImg, &frontCar);
    lv_obj_align(frontCarImg, LV_ALIGN_CENTER, -40, 0);

    sideCarImg = lv_img_create(backgroundImg);
    lv_img_set_src(sideCarImg, &sideCar);
    lv_obj_align(sideCarImg, LV_ALIGN_CENTER, 40, 0);
}

//DEBUG
//int counter = 0;
//bool flag = false;

void vRefreshIMUDataHandler( void )
{
    if (RefreshIMUDataHandlerFlag)
    {
        if( xDisplaySemaphore != NULL )
        {
            if( xSemaphoreTake( xDisplaySemaphore, (WAIT_TIMEOUT) / portTICK_PERIOD_MS ) == pdTRUE )
            {
                const FusionEuler *angles = pGetEulerAngles();
                actualEulerAngles.angle.roll = angles->angle.roll;
                actualEulerAngles.angle.pitch = angles->angle.pitch;
                //actualEulerAngles.angle.yaw = angles->angle.yaw;
                xSemaphoreGive( xDisplaySemaphore );
            }

            actualEulerAngles.angle.roll -= rollCalibVal;
            actualEulerAngles.angle.pitch -= pitchCalibVal;
            actualEulerAngles.angle.pitch *= -1.0f;

            lv_arc_set_value(indicatorLeft, 50 + actualEulerAngles.angle.roll);
            lv_arc_set_value(indicatorRight, 50 + actualEulerAngles.angle.pitch );
            lv_img_set_angle(sideCarImg, (int16_t)actualEulerAngles.angle.pitch * 10);
            lv_img_set_angle(frontCarImg, (int16_t)actualEulerAngles.angle.roll * 10);
            

            char rollText[5];
            char pichText[5];

            sprintf(rollText,"%0.1f\n", actualEulerAngles.angle.roll);
            sprintf(pichText,"%0.1f\n", actualEulerAngles.angle.pitch);

            lv_label_set_text(txtRollTile2, rollText);
            lv_label_set_text(txtPitchTile2, pichText);

            actualEulerAngles.angle.roll += 50.0;
            actualEulerAngles.angle.pitch += 50.0;
            
            if ( actualEulerAngles.angle.roll <= 5.0f )
            {
                lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff0000), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtRollTile2, lv_color_hex(0xff0000), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.roll <= 15.0f && actualEulerAngles.angle.roll > 5.0f )
            {
                lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff5757), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtRollTile2, lv_color_hex(0xff5757), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.roll <= 25.0f && actualEulerAngles.angle.roll > 15.0f )
            {
                lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff7a28), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtRollTile2, lv_color_hex(0xff7a28), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.roll <= 70.0f && actualEulerAngles.angle.roll > 25.0f )
            {
                lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0x00ff00), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtRollTile2, lv_color_hex(0x00ff00), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.roll <= 80.0f && actualEulerAngles.angle.roll > 70.0f )
            {
                lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff7a28), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtRollTile2, lv_color_hex(0xff7a28), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.roll <= 90.0f && actualEulerAngles.angle.roll > 80.0f )
            {
                lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff5757), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtRollTile2, lv_color_hex(0xff5757), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.roll <= 100.0f && actualEulerAngles.angle.roll > 90.0f )
            {
                lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff0000), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtRollTile2, lv_color_hex(0xff0000), LV_PART_MAIN);
            }


            if ( actualEulerAngles.angle.pitch <= 5.0f )
            {
                lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff0000), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtPitchTile2, lv_color_hex(0xff0000), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.pitch <= 15.0f && actualEulerAngles.angle.pitch > 5.0f )
            {
                lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff5757), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtPitchTile2, lv_color_hex(0xff5757), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.pitch <= 25.0f && actualEulerAngles.angle.pitch > 15.0f )
            {
                lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff7a28), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtPitchTile2, lv_color_hex(0xff7a28), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.pitch <= 70.0f && actualEulerAngles.angle.pitch > 25.0f )
            {
                lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0x00ff00), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtPitchTile2, lv_color_hex(0x00ff00), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.pitch <= 80.0f && actualEulerAngles.angle.pitch > 70.0f )
            {
                lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff7a28), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtPitchTile2, lv_color_hex(0xff7a28), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.pitch <= 90.0f && actualEulerAngles.angle.pitch > 80.0f )
            {
                lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff5757), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtPitchTile2, lv_color_hex(0xff5757), LV_PART_MAIN);
            }
            else if ( actualEulerAngles.angle.pitch <= 100.0f && actualEulerAngles.angle.pitch > 90.0f )
            {
                lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff0000), LV_PART_INDICATOR);
                lv_obj_set_style_text_color(txtPitchTile2, lv_color_hex(0xff0000), LV_PART_MAIN);
            }

            //DEBUG!!!
            // lv_arc_set_value(indicatorLeft, counter);
            // lv_arc_set_value(indicatorRight, counter);

            // if ( counter <= 5 )
            // {
            //     lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff0000), LV_PART_INDICATOR);
            //     lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff0000), LV_PART_INDICATOR);
            //     lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0xff0000), LV_PART_MAIN);
            // }
            // else if ( counter <= 15 && counter > 5 )
            // {
            //     lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff5757), LV_PART_INDICATOR);
            //     lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff5757), LV_PART_INDICATOR);
            //     lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0xff5757), LV_PART_MAIN);
            // }
            // else if ( counter <= 25 && counter > 15 )
            // {
            //     lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff7a28), LV_PART_INDICATOR);
            //     lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff7a28), LV_PART_INDICATOR);
            //     lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0xff7a28), LV_PART_MAIN);
            // }
            // else if ( counter <= 70 && counter > 25 )
            // {
            //     lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0x00ff00), LV_PART_INDICATOR);
            //     lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0x00ff00), LV_PART_INDICATOR);
            //     lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0x00ff00), LV_PART_MAIN);
            // }
            // else if ( counter <= 80 && counter > 70 )
            // {
            //     lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff7a28), LV_PART_INDICATOR);
            //     lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff7a28), LV_PART_INDICATOR);
            //     lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0xff7a28), LV_PART_MAIN);
            // }
            // else if ( counter <= 90 && counter > 80 )
            // {
            //     lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff5757), LV_PART_INDICATOR);
            //     lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff5757), LV_PART_INDICATOR);
            //     lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0xff5757), LV_PART_MAIN);
            // }
            // else if ( counter <= 100 && counter > 90 )
            // {
            //     lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0xff0000), LV_PART_INDICATOR);
            //     lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0xff0000), LV_PART_INDICATOR);
            //     lv_obj_set_style_text_color(backgroundImg, lv_color_hex(0xff0000), LV_PART_MAIN);
            // }

            // lv_img_set_angle(sideCarImg, counter * 10);
            // lv_img_set_angle(frontCarImg, counter * 10);

            // if(counter<100 && flag == false)
            // {
            //     counter++;
            // }
            // else if(counter == 100 && flag == false)
            // {
            //     flag = true;
            // }
            // else if(counter > 0 && flag == true)
            // {
            //     counter--;
            // }
            // else if(counter == 0 && flag == true)
            // {
            //     flag = false;
            // }
            
            vUnsetRefreshIMUDataHandlerFlag();
        }
    }
}

void vChangeToMainScreen( void )
{
    lv_obj_set_tile(tileView, tile2, LV_ANIM_OFF);
}

void vSetCalibVal( float rollVal, float pitchVal )
{
    rollCalibVal = rollVal;
    pitchCalibVal = pitchVal;
}

static void vSetRefreshIMUDataHandlerFlag()
{
    RefreshIMUDataHandlerFlag = true;
}

static void vUnsetRefreshIMUDataHandlerFlag()
{
    RefreshIMUDataHandlerFlag = false;
}

static bool bLvglTimerCallback( struct repeating_timer *t )
{
    UNUSED( t );

    lv_tick_inc( 5 );
    return true;
}

static bool bRefreshIMUDataTimerCallback( struct repeating_timer *t )
{
    UNUSED( t );

    vSetRefreshIMUDataHandlerFlag();

    return true;
}

static void vDispFlushCb( lv_disp_drv_t * pDisp, const lv_area_t * pArea, lv_color_t * pColor )
{
    UNUSED( pDisp );

    LCD_1IN28_SetWindows( pArea->x1, pArea->y1, pArea->x2 , pArea->y2 );
    dma_channel_configure( dma_tx,
                          &c,
                          &spi_get_hw(LCD_SPI_PORT)->dr, 
                          pColor, // read address
                          ((pArea->x2 + 1 - pArea-> x1)*(pArea->y2 + 1 - pArea -> y1))*2,
                          true);
}

static void vDmaHandler( void )
{
    if ( dma_channel_get_irq0_status(dma_tx) ) 
    {
        dma_channel_acknowledge_irq0(dma_tx);
        lv_disp_flush_ready(&xLvglDispDrv);         /* Indicate you are ready with the flushing*/
    }
}