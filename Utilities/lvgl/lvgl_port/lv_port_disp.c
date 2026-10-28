/**
 * @file lv_port_disp_template.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 240 is used for now.
    #define MY_DISP_HOR_RES    240
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_VER_RES with the actual screen height, default value 320 is used for now.
    #define MY_DISP_VER_RES    320
#endif

/* Number of lines in each partial draw buffer (same as v8 port) */
#ifndef LV_DISP_BUF_LINES
    #define LV_DISP_BUF_LINES  40
#endif

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
#define DISP_BUF_SIZE  (MY_DISP_HOR_RES * LV_DISP_BUF_LINES * BYTE_PER_PIXEL)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
/* Two buffers for partial rendering, flushed by DMA in the background.
 * uint32_t arrays keep the buffers aligned to LV_DRAW_BUF_ALIGN (4 bytes). */
LV_ATTRIBUTE_MEM_ALIGN
static uint32_t buf_2_1[DISP_BUF_SIZE / sizeof(uint32_t)];

LV_ATTRIBUTE_MEM_ALIGN
static uint32_t buf_2_2[DISP_BUF_SIZE / sizeof(uint32_t)];

/* Display waiting for the DMA transfer complete interrupt */
static lv_display_t * volatile pending_disp = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, disp_flush);

    /* Two buffers for partial rendering
     * In flush_cb DMA is used to update the display in the background.*/
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

/* Call from the LCD DMA transfer complete interrupt */
void lv_port_disp_flush_complete(void)
{
    lv_display_t * disp = pending_disp;

    if(disp != NULL) {
        pending_disp = NULL;
        /*Inform the graphics library that you are ready with the flushing*/
        lv_display_flush_ready(disp);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    lcd_dma_init();
    lcd_init();
    lcd_clear(LCD_COLOR_BLACK);
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *The transfer is done by DMA in the background, 'lv_display_flush_ready()' is called
 *in lv_port_disp_flush_complete() when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    if(!disp_flush_enabled) {
        lv_display_flush_ready(disp_drv);
        return;
    }

    pending_disp = disp_drv;
    lcd_picture_draw_dma((uint16_t)area->x1, (uint16_t)area->y1,
                         (uint16_t)area->x2, (uint16_t)area->y2,
                         (const uint16_t *)px_map);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
