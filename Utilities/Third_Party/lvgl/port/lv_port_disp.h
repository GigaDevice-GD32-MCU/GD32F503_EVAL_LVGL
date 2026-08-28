/*
 * F503 LVGL v8 display port header
 */

#ifndef LV_PORT_DISP_V8_H
#define LV_PORT_DISP_V8_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "../../Utilities/BSP_Drive/gd32f503v_lcd_eval.h"

#define XSIZE_PHYS 240
#define YSIZE_PHYS 320

void lv_port_disp_init(void);
void disp_enable_update(void);
void disp_disable_update(void);
void lv_port_disp_flush_complete(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_DISP_V8_H */
