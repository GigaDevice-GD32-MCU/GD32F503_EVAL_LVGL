#include "lv_port_disp.h"

#ifndef LV_DISP_BUF_LINES
#define LV_DISP_BUF_LINES 40
#endif

#define LV_DISP_BUF_PIXELS (XSIZE_PHYS * LV_DISP_BUF_LINES)

static lv_color_t buf_1[LV_DISP_BUF_PIXELS];
static lv_color_t buf_2[LV_DISP_BUF_PIXELS];
static lv_disp_draw_buf_t draw_buf;
static bool disp_flush_enabled = true;
static volatile lv_disp_drv_t *pending_disp_drv = NULL;

static void disp_init(void);
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *px_map);

void lv_port_disp_init(void)
{
    static lv_disp_drv_t disp_drv;

    disp_init();

    lv_disp_draw_buf_init(&draw_buf, buf_1, buf_2, LV_DISP_BUF_PIXELS);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = XSIZE_PHYS;
    disp_drv.ver_res = YSIZE_PHYS;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);
}

static void disp_init(void)
{
    lcd_dma_init();
    lcd_init();
    lcd_clear(LCD_COLOR_BLACK);
}

void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *px_map)
{
    uint16_t dst_x1;
    uint16_t dst_y1;
    uint16_t dst_x2;
    uint16_t dst_y2;

    if(!disp_flush_enabled) {
        lv_disp_flush_ready(disp_drv);
        return;
    }

    dst_x1 = (uint16_t)area->x1;
    dst_y1 = (uint16_t)area->y1;
    dst_x2 = (uint16_t)area->x2;
    dst_y2 = (uint16_t)area->y2;

    pending_disp_drv = disp_drv;
    lcd_picture_draw_dma(dst_x1, dst_y1, dst_x2, dst_y2, (const uint16_t *)px_map);
}

void lv_port_disp_flush_complete(void)
{
    if(pending_disp_drv != NULL) {
        lv_disp_drv_t *disp_drv = (lv_disp_drv_t *)pending_disp_drv;
        pending_disp_drv = NULL;
        lv_disp_flush_ready(disp_drv);
    }
}
