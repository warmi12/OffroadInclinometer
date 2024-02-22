#include "DEV_Config.h"
#include "LCD_1in28.h"
#include "lvgl.h"

#include "LvglProcess.h"
#include "IMUTask.h"
#include "ImageData.h"

#define UNUSED(x) ((void)(x))
#define DISP_HOR_RES 240
#define DISP_VER_RES 240

static struct repeating_timer xLvglTimer;
static struct repeating_timer xIMUTimer;

static lv_obj_t * pLabel; 
static lv_disp_draw_buf_t xLvglDispBuf;
static lv_color_t xLvglDispBuf0[ DISP_HOR_RES * (DISP_VER_RES/2) ];
static lv_color_t xLvglDispBuf1[ DISP_HOR_RES * (DISP_VER_RES/2) ];
static lv_disp_drv_t xLvglDispDrv;

static bool bLvglTimerCallback( struct repeating_timer *t );
static bool bIMUTimerCallback( struct repeating_timer *t );

static void vDispFlushCb( lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p );
static void vDmaHandler( void );

void vLvglInit( void )
{
    add_repeating_timer_ms( 5, bLvglTimerCallback, NULL, &xLvglTimer );
    add_repeating_timer_ms( 500, bIMUTimerCallback, NULL, &xIMUTimer );

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

static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_x(var, v);
}

static void anim_size_cb(void * var, int32_t v)
{
    lv_obj_set_size(var, v, v);
}

void vWidgetsInit( void )
{
    pLabel = lv_label_create(lv_scr_act());

    /* IMAGE */

    LV_IMG_DECLARE(pic);
    //lv_obj_t *img1 = lv_img_create(tile1);
    lv_obj_t *img1 = lv_img_create(pLabel);
    lv_img_set_src(img1, &pic);
    lv_obj_set_pos(img1, 0, 0);

    /* INDICATORS */

    lv_obj_t * indicatorLeft = lv_arc_create(lv_scr_act());
    
    lv_obj_set_size(indicatorLeft, 170, 170);
    lv_obj_remove_style(indicatorLeft, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(indicatorLeft, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_set_pos(indicatorLeft, 30, 35);

    lv_arc_set_rotation(indicatorLeft, 130);
    lv_arc_set_bg_angles(indicatorLeft, 0, 100);
    lv_obj_set_style_arc_width(indicatorLeft, 10, 0);
    lv_obj_set_style_arc_width(indicatorLeft, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(indicatorLeft, lv_color_hex(0x003a57), 0);
    lv_arc_set_value(indicatorLeft, 75);
    lv_arc_set_mode(indicatorLeft, LV_ARC_MODE_SYMMETRICAL);
    
    lv_anim_t indicatorLeftAnim;
    lv_anim_init(&indicatorLeftAnim);
    lv_anim_set_var(&indicatorLeftAnim, indicatorLeft);
    //lv_anim_set_exec_cb(&a, set_angle);
    lv_anim_set_values(&indicatorLeftAnim, 0, 100);
    lv_anim_start(&indicatorLeftAnim);


    lv_obj_t * indicatorRight = lv_arc_create(lv_scr_act());
    
    lv_obj_set_size(indicatorRight, 170, 170);
    lv_obj_remove_style(indicatorRight, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(indicatorRight, LV_OBJ_FLAG_CLICKABLE);  /*To not allow adjusting by click*/
    lv_obj_set_pos(indicatorRight, 40, 35);

    lv_arc_set_rotation(indicatorRight, 310);
    lv_arc_set_bg_angles(indicatorRight, 0, 100);
    lv_obj_set_style_arc_width(indicatorRight, 10, 0);
    lv_obj_set_style_arc_width(indicatorRight, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(indicatorRight, lv_color_hex(0x003a57), 0);
    lv_arc_set_value(indicatorRight, 75);
    lv_arc_set_mode(indicatorRight, LV_ARC_MODE_SYMMETRICAL);
    
   //lv_obj_add_event_cb(arc, value_changed_event_cb, LV_EVENT_VALUE_CHANGED, label);

    //lv_obj_send_event(arc, LV_EVENT_VALUE_CHANGED, NULL);
    lv_anim_t indicatorRightAnim;
    lv_anim_init(&indicatorRightAnim);
    lv_anim_set_var(&indicatorRightAnim, indicatorRight);
    //lv_anim_set_exec_cb(&a, set_angle);
    lv_anim_set_values(&indicatorRightAnim, 0, 100);
    lv_anim_start(&indicatorRightAnim);
}

static bool bLvglTimerCallback( struct repeating_timer *t )
{
    UNUSED( t );

    lv_tick_inc( 5 );
    return true;
}

static bool bIMUTimerCallback( struct repeating_timer *t )
{
    UNUSED( t );

    //if(MUTEX/SEM)?
    //get_eulerdata();
    char label_text[64];
    const FusionEuler *angles = pGetEulerAngles();
    sprintf(label_text,"%4.1f",angles->angle.roll);
    lv_label_set_text(pLabel,label_text);
    
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