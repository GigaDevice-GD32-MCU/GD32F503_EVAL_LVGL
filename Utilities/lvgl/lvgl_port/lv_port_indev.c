/**
 * @file lv_port_indev_template.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lv_port_disp.h"
#include "touch_panel.h"

/*********************
 *      DEFINES
 *********************/
#ifndef TOUCH_PROBE_DEBUG
    #define TOUCH_PROBE_DEBUG   0
#endif

/* v8: LV_INDEV_DEF_READ_PERIOD = 30 ms */
#define TOUCH_READ_PERIOD   30

enum {
    TOUCH_X_MIN = 360,
    TOUCH_X_MAX = 3816,
    TOUCH_Y_MIN = 435,
    TOUCH_Y_MID = 2316,
    TOUCH_Y_MAX = 3900,
    TOUCH_Y_OFFSET = 6
};

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);
static bool touchpad_is_pressed(void);
static bool touchpad_get_xy(int32_t * x, int32_t * y);
static bool touchpad_get_raw_ad(uint16_t * ad_x, uint16_t * ad_y);
static int32_t touchpad_map_ad(uint16_t ad, uint16_t ad_min, uint16_t ad_max, int32_t screen_size);
static int32_t touchpad_map_portrait_y(uint16_t ad_y);
#if TOUCH_PROBE_DEBUG
static void touchpad_probe_update(lv_indev_state_t state, int32_t x, int32_t y);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad;

static FlagStatus touch_irq_idle = SET;
#if TOUCH_PROBE_DEBUG
static lv_obj_t * touch_probe_label = NULL;
static lv_obj_t * touch_probe_cursor = NULL;
static uint16_t touch_dbg_ad_x = 0;
static uint16_t touch_dbg_ad_y = 0;
static uint16_t touch_dbg_last_ad_x = 0;
static uint16_t touch_dbg_last_ad_y = 0;
static int32_t touch_dbg_last_x = 0;
static int32_t touch_dbg_last_y = 0;
static uint8_t touch_dbg_src = 0;
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);

    /*Keep the v8 input read period*/
    lv_timer_set_period(lv_indev_get_read_timer(indev_touchpad), TOUCH_READ_PERIOD);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad: detect the idle level of the PEN IRQ pin*/
static void touchpad_init(void)
{
    uint8_t i;
    uint8_t set_cnt = 0;
    uint8_t reset_cnt = 0;

    for(i = 0; i < 16; i++) {
        if(touch_pen_irq() == SET) {
            set_cnt++;
        }
        else {
            reset_cnt++;
        }
    }

    touch_irq_idle = (set_cnt >= reset_cnt) ? SET : RESET;
    (void)touch_irq_idle;
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static int32_t last_x = 0;
    static int32_t last_y = 0;

    (void)indev_drv;

    /*Save the pressed coordinates and the state*/
    if(touchpad_get_xy(&last_x, &last_y)) {
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;

#if TOUCH_PROBE_DEBUG
    touchpad_probe_update(data->state, last_x, last_y);
#endif
}

/*Return true is the touchpad is pressed*/
static bool touchpad_is_pressed(void)
{
    return (touch_pen_irq() == RESET);
}

/*Get the x and y coordinates if the touchpad is pressed*/
static bool touchpad_get_xy(int32_t * x, int32_t * y)
{
    uint8_t retry;
    uint16_t ad_x = 0;
    uint16_t ad_y = 0;
    bool got_ad = false;
    int32_t lv_x;
    int32_t lv_y;

    if(!touchpad_is_pressed()) {
#if TOUCH_PROBE_DEBUG
        touch_dbg_src = 0U;
        touch_dbg_ad_x = 0U;
        touch_dbg_ad_y = 0U;
#endif
        return false;
    }

    for(retry = 0; retry < 20; retry++) {
        if(touch_ad_xy_get(&ad_x, &ad_y) == SUCCESS) {
            got_ad = true;
#if TOUCH_PROBE_DEBUG
            touch_dbg_src = 1U;
#endif
            break;
        }
    }

    /* Some panels may return SUCCESS with zero AD values. Treat as invalid. */
    if(got_ad && ((ad_x == 0U) || (ad_y == 0U))) {
        got_ad = false;
    }

    if(!got_ad) {
        ad_x = touch_average_ad_x_get();
        ad_y = touch_average_ad_y_get();
        if((ad_x > 0U) && (ad_y > 0U)) {
            got_ad = true;
#if TOUCH_PROBE_DEBUG
            touch_dbg_src = 3U;
#endif
        }
    }

    if(!got_ad) {
        got_ad = touchpad_get_raw_ad(&ad_x, &ad_y);
#if TOUCH_PROBE_DEBUG
        if(got_ad) {
            touch_dbg_src = 2U;
        }
#endif
    }

    if(!got_ad) {
#if TOUCH_PROBE_DEBUG
        touch_dbg_src = 0U;
        touch_dbg_ad_x = 0U;
        touch_dbg_ad_y = 0U;
#endif
        return false;
    }

#if TOUCH_PROBE_DEBUG
    touch_dbg_ad_x = ad_x;
    touch_dbg_ad_y = ad_y;
#endif

    lv_x = touchpad_map_ad(ad_x, TOUCH_X_MIN, TOUCH_X_MAX, MY_DISP_HOR_RES);
    lv_y = touchpad_map_portrait_y(ad_y);

    lv_x = MY_DISP_HOR_RES - 1 - lv_x;
    lv_y = MY_DISP_VER_RES - 1 - lv_y - TOUCH_Y_OFFSET;

    if(lv_x < 0) {
        lv_x = 0;
    }
    else if(lv_x >= MY_DISP_HOR_RES) {
        lv_x = MY_DISP_HOR_RES - 1;
    }

    if(lv_y < 0) {
        lv_y = 0;
    }
    else if(lv_y >= MY_DISP_VER_RES) {
        lv_y = MY_DISP_VER_RES - 1;
    }

    (*x) = lv_x;
    (*y) = lv_y;
#if TOUCH_PROBE_DEBUG
    touch_dbg_last_ad_x = ad_x;
    touch_dbg_last_ad_y = ad_y;
    touch_dbg_last_x = lv_x;
    touch_dbg_last_y = lv_y;
#endif
    return true;
}

/*Linear map of an AD value to [0, screen_size - 1]*/
static int32_t touchpad_map_ad(uint16_t ad, uint16_t ad_min, uint16_t ad_max, int32_t screen_size)
{
    if(ad <= ad_min) {
        return 0;
    }

    if(ad >= ad_max) {
        return screen_size - 1;
    }

    return (int32_t)(((uint32_t)(ad - ad_min) * (uint32_t)(screen_size - 1)) /
                     (uint32_t)(ad_max - ad_min));
}

/*Two-segment Y mapping (portrait) calibrated with TOUCH_Y_MID*/
static int32_t touchpad_map_portrait_y(uint16_t ad_y)
{
    int32_t middle_y = (MY_DISP_VER_RES / 2) - 1;

    if(ad_y <= TOUCH_Y_MIN) {
        return 0;
    }

    if(ad_y >= TOUCH_Y_MAX) {
        return MY_DISP_VER_RES - 1;
    }

    if(ad_y <= TOUCH_Y_MID) {
        return touchpad_map_ad(ad_y, TOUCH_Y_MIN, TOUCH_Y_MID, middle_y + 1);
    }

    return middle_y + touchpad_map_ad(ad_y, TOUCH_Y_MID, TOUCH_Y_MAX,
                                      MY_DISP_VER_RES - middle_y);
}

/*Fallback: read the raw AD values directly over SPI*/
static bool touchpad_get_raw_ad(uint16_t * ad_x, uint16_t * ad_y)
{
    uint8_t retry;
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
    uint16_t dx;
    uint16_t dy;
    uint16_t avg_x;
    uint16_t avg_y;

    if(!touchpad_is_pressed()) {
        return false;
    }

    for(retry = 0; retry < 8; retry++) {
        touch_start();
        touch_write(CH_X);
        x1 = touch_read();

        touch_start();
        touch_write(CH_Y);
        y1 = touch_read();

        touch_start();
        touch_write(CH_X);
        x2 = touch_read();

        touch_start();
        touch_write(CH_Y);
        y2 = touch_read();

        dx = (x1 > x2) ? (uint16_t)(x1 - x2) : (uint16_t)(x2 - x1);
        dy = (y1 > y2) ? (uint16_t)(y1 - y2) : (uint16_t)(y2 - y1);

        if((dx > 500U) || (dy > 500U)) {
            continue;
        }

        avg_x = (uint16_t)((x1 + x2) / 2U);
        avg_y = (uint16_t)((y1 + y2) / 2U);

        if((avg_x == 0U) || (avg_y == 0U)) {
            continue;
        }

        *ad_x = avg_x;
        *ad_y = avg_y;
        return true;
    }

    return false;
}

#if TOUCH_PROBE_DEBUG
/*On-screen touch debug overlay: label with AD/XY info and a cursor dot*/
static void touchpad_probe_update(lv_indev_state_t state, int32_t x, int32_t y)
{
    static uint16_t update_div = 0;
    char src_ch;
    FlagStatus irq_now;
    lv_obj_t * scr;

    update_div++;
    if((update_div % 3U) != 0U) {
        return;
    }

    if(touch_probe_label == NULL) {
        scr = lv_screen_active();
        touch_probe_label = lv_label_create(scr);
        lv_obj_set_style_bg_opa(touch_probe_label, LV_OPA_70, 0);
        lv_obj_set_style_bg_color(touch_probe_label, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_color(touch_probe_label, lv_color_hex(0x00FF66), 0);
        lv_obj_set_style_pad_left(touch_probe_label, 4, 0);
        lv_obj_set_style_pad_right(touch_probe_label, 4, 0);
        lv_obj_set_style_pad_top(touch_probe_label, 2, 0);
        lv_obj_set_style_pad_bottom(touch_probe_label, 2, 0);
        lv_obj_set_pos(touch_probe_label, 4, 4);
        lv_obj_move_foreground(touch_probe_label);
    }

    if(touch_probe_cursor == NULL) {
        scr = lv_screen_active();
        touch_probe_cursor = lv_obj_create(scr);
        lv_obj_set_size(touch_probe_cursor, 9, 9);
        lv_obj_set_style_radius(touch_probe_cursor, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(touch_probe_cursor, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(touch_probe_cursor, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_border_width(touch_probe_cursor, 1, 0);
        lv_obj_set_style_border_color(touch_probe_cursor, lv_color_hex(0xFFFFFF), 0);
        lv_obj_remove_flag(touch_probe_cursor, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(touch_probe_cursor, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_move_foreground(touch_probe_cursor);
    }

    if(state == LV_INDEV_STATE_PRESSED) {
        lv_obj_remove_flag(touch_probe_cursor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(touch_probe_cursor, x - 4, y - 4);
        lv_obj_move_foreground(touch_probe_cursor);
    }
    else {
        lv_obj_add_flag(touch_probe_cursor, LV_OBJ_FLAG_HIDDEN);
    }

    src_ch = (touch_dbg_src == 1U) ? 'F' : ((touch_dbg_src == 2U) ? 'R' : ((touch_dbg_src == 3U) ? 'A' : '-'));
    irq_now = touch_pen_irq();

    lv_label_set_text_fmt(touch_probe_label,
                          "TP %s irq:%u idle:%u src:%c\nad:%u,%u xy:%ld,%ld\nlast:%u,%u -> %ld,%ld",
                          (state == LV_INDEV_STATE_PRESSED) ? "P" : "R",
                          (unsigned int)irq_now,
                          (unsigned int)touch_irq_idle,
                          src_ch,
                          (unsigned int)touch_dbg_ad_x,
                          (unsigned int)touch_dbg_ad_y,
                          (long)x,
                          (long)y,
                          (unsigned int)touch_dbg_last_ad_x,
                          (unsigned int)touch_dbg_last_ad_y,
                          (long)touch_dbg_last_x,
                          (long)touch_dbg_last_y);
}
#endif /*TOUCH_PROBE_DEBUG*/

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
