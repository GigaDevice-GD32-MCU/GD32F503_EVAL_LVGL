#include "lv_port_indev.h"
#include "lv_port_disp.h"
#include "../../../BSP_Drive/touch_panel.h"

#ifndef TOUCH_PROBE_DEBUG
#define TOUCH_PROBE_DEBUG 0
#endif

enum {
    TOUCH_X_MIN = 360,
    TOUCH_X_MAX = 3816,
    TOUCH_Y_MIN = 435,
    TOUCH_Y_MID = 2316,
    TOUCH_Y_MAX = 3900,
    TOUCH_Y_OFFSET = 6
};

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
static bool touchpad_is_pressed(void);
static bool touchpad_get_xy(int32_t *x, int32_t *y);
static void touchpad_irq_idle_init(void);
static bool touchpad_get_raw_ad(uint16_t *ad_x, uint16_t *ad_y);
static int32_t touchpad_map_ad(uint16_t ad, uint16_t ad_min, uint16_t ad_max, int32_t screen_size);
static int32_t touchpad_map_portrait_y(uint16_t ad_y);
#if TOUCH_PROBE_DEBUG
static void touchpad_probe_update(lv_indev_state_t state, int32_t x, int32_t y);
#endif

static lv_indev_drv_t indev_touchpad_drv;
static lv_indev_t *indev_touchpad = NULL;
static FlagStatus touch_irq_idle = SET;
static bool touch_irq_idle_inited = false;
#if TOUCH_PROBE_DEBUG
static lv_obj_t *touch_probe_label = NULL;
static lv_obj_t *touch_probe_cursor = NULL;
static uint16_t touch_dbg_ad_x = 0;
static uint16_t touch_dbg_ad_y = 0;
static uint16_t touch_dbg_last_ad_x = 0;
static uint16_t touch_dbg_last_ad_y = 0;
static int32_t touch_dbg_last_x = 0;
static int32_t touch_dbg_last_y = 0;
static uint8_t touch_dbg_src = 0;
#endif

void lv_port_indev_init(void)
{
    touchpad_irq_idle_init();

    lv_indev_drv_init(&indev_touchpad_drv);
    indev_touchpad_drv.type = LV_INDEV_TYPE_POINTER;
    indev_touchpad_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_touchpad_drv);
    (void)indev_touchpad;
}

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static int32_t last_x = 0;
    static int32_t last_y = 0;
    bool got_xy;

    (void)indev_drv;

    got_xy = touchpad_get_xy(&last_x, &last_y);

    if(got_xy) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = last_x;
        data->point.y = last_y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        last_x = 0;
        last_y = 0;
        data->point.x = 0;
        data->point.y = 0;
    }
#if TOUCH_PROBE_DEBUG
    touchpad_probe_update(data->state, last_x, last_y);
#endif
}

static bool touchpad_is_pressed(void)
{
    return (touch_pen_irq() == RESET);
}

static bool touchpad_get_xy(int32_t *x, int32_t *y)
{
    uint8_t retry;
    uint16_t ad_x = 0;
    uint16_t ad_y = 0;
    bool got_ad = false;
    int32_t tp_x;
    int32_t tp_y;
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
        if(got_ad) {
#if TOUCH_PROBE_DEBUG
            touch_dbg_src = 2U;
#endif
        }
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

    tp_x = touchpad_map_ad(ad_x, TOUCH_X_MIN, TOUCH_X_MAX, XSIZE_PHYS);
    tp_y = touchpad_map_portrait_y(ad_y);

    lv_x = tp_x;
    lv_y = tp_y;
    lv_x = XSIZE_PHYS - 1 - lv_x;
    lv_y = YSIZE_PHYS - 1 - lv_y - TOUCH_Y_OFFSET;

    if(lv_x < 0) {
        lv_x = 0;
    } else if(lv_x >= XSIZE_PHYS) {
        lv_x = XSIZE_PHYS - 1;
    }

    if(lv_y < 0) {
        lv_y = 0;
    } else if(lv_y >= YSIZE_PHYS) {
        lv_y = YSIZE_PHYS - 1;
    }

    *x = lv_x;
    *y = lv_y;
#if TOUCH_PROBE_DEBUG
    touch_dbg_last_ad_x = ad_x;
    touch_dbg_last_ad_y = ad_y;
    touch_dbg_last_x = lv_x;
    touch_dbg_last_y = lv_y;
#endif
    return true;
}

static int32_t touchpad_map_ad(uint16_t ad, uint16_t ad_min, uint16_t ad_max, int32_t screen_size)
{
    int32_t value;

    if(ad <= ad_min) {
        return 0;
    }

    if(ad >= ad_max) {
        return screen_size - 1;
    }

    value = (int32_t)(((uint32_t)(ad - ad_min) * (uint32_t)(screen_size - 1)) /
                      (uint32_t)(ad_max - ad_min));

    return value;
}

static int32_t touchpad_map_portrait_y(uint16_t ad_y)
{
    int32_t middle_y = (YSIZE_PHYS / 2) - 1;

    if(ad_y <= TOUCH_Y_MIN) {
        return 0;
    }

    if(ad_y >= TOUCH_Y_MAX) {
        return YSIZE_PHYS - 1;
    }

    if(ad_y <= TOUCH_Y_MID) {
        return touchpad_map_ad(ad_y, TOUCH_Y_MIN, TOUCH_Y_MID, middle_y + 1);
    }

    return middle_y + touchpad_map_ad(ad_y, TOUCH_Y_MID, TOUCH_Y_MAX,
                                      YSIZE_PHYS - middle_y);
}

static void touchpad_irq_idle_init(void)
{
    uint8_t i;
    uint8_t set_cnt = 0;
    uint8_t reset_cnt = 0;

    for(i = 0; i < 16; i++) {
        if(touch_pen_irq() == SET) {
            set_cnt++;
        } else {
            reset_cnt++;
        }
    }

    touch_irq_idle = (set_cnt >= reset_cnt) ? SET : RESET;
    touch_irq_idle_inited = true;
    (void)touch_irq_idle_inited;
}

static bool touchpad_get_raw_ad(uint16_t *ad_x, uint16_t *ad_y)
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
static void touchpad_probe_update(lv_indev_state_t state, int32_t x, int32_t y)
{
    static uint16_t update_div = 0;
    char src_ch;
    FlagStatus irq_now;
    lv_obj_t *scr;

    update_div++;
    if((update_div % 3U) != 0U) {
        return;
    }

    if(touch_probe_label == NULL) {
        scr = lv_scr_act();
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
        scr = lv_scr_act();
        touch_probe_cursor = lv_obj_create(scr);
        lv_obj_set_size(touch_probe_cursor, 9, 9);
        lv_obj_set_style_radius(touch_probe_cursor, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(touch_probe_cursor, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(touch_probe_cursor, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_border_width(touch_probe_cursor, 1, 0);
        lv_obj_set_style_border_color(touch_probe_cursor, lv_color_hex(0xFFFFFF), 0);
        lv_obj_clear_flag(touch_probe_cursor, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(touch_probe_cursor, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_move_foreground(touch_probe_cursor);
    }

    if(state == LV_INDEV_STATE_PRESSED) {
        lv_obj_clear_flag(touch_probe_cursor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(touch_probe_cursor, x - 4, y - 4);
        lv_obj_move_foreground(touch_probe_cursor);
    } else {
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
#endif
