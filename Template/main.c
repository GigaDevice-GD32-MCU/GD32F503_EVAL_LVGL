/*!
    \file    main.c
    \brief   LVGL application entry

    \version 2026-02-25, V1.0.4, demo for GD32F50x
*/

/*
    Copyright (c) 2026, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "gd32f50x.h"
#include "systick.h"
#include "gd32f503v_lcd_eval.h"
#include "../Utilities/BSP_Drive/gd32f503v_eval.h"
#include "touch_panel.h"

#include "lvgl.h"
#include "../Utilities/Third_Party/lvgl/demos/lv_demos.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
int main(void)
{
    /* initialize UART for printf debugging */
    gd_eval_com_init(EVAL_COM0);

    /* configure systick */
    systick_config();

    /* configure the EXMC access mode for LCD */
    exmc_lcd_init();

    /* configure the GPIO of SPI touch panel */
    touch_panel_gpio_configure();
    delay_1ms(50);

    /* initialize the LCD */
    lcd_init();

    /* LVGL bring-up */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_demo_widgets();
    while(1) {
        lv_timer_handler();
    }
}
