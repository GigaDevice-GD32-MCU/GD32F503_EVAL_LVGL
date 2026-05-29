/*!
    \file  gd32f503v_lcd_eval.c
    \brief LCD driver functions

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
#include "gd32f503v_lcd_eval.h"


#define LCD_DMA                      DMA1
#define LCD_DMA_CHANNEL              DMA_CH0
#define LCD_DMA_IRQn                 DMA1_Channel0_IRQn

#define LCD_ILI9320       0x9320
#define LCD_ILI9325       0x9325

#define ABS(X)  ((X) > 0 ? (X) : -(X))

uint16_t device_code;
static volatile FlagStatus lcd_dma_busy = RESET;

/*!
    \brief      lcd peripheral initialize
    \param[in]  none 
    \param[out] none
    \retval     none
*/
void exmc_lcd_init(void)
{
    exmc_norsram_parameter_struct lcd_init_struct;
    exmc_norsram_timing_parameter_struct lcd_timing_init_struct;

    /* EXMC clock enable */
    rcu_periph_clock_enable(RCU_EXMC);

    /* GPIO clock enable */
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);

    /* configure GPIO EXMC_D[0~3]  EXMC_D[13~15] */
    gpio_af_set(GPIOD, GPIO_AF_1, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);

    /* configure GPIO EXMC_D[4~12]) */
    gpio_af_set(GPIOE, GPIO_AF_1, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | 
                                  GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | 
                                  GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | 
                                                       GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | 
                                                       GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | 
                                                                      GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | 
                                                                      GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    /* configure PE2(EXMC_A23) */
    gpio_af_set(GPIOE, GPIO_AF_1, GPIO_PIN_2);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2, GPIO_PIN_2);
    /* configure NOE and NWE */
    gpio_af_set(GPIOD, GPIO_AF_1, GPIO_PIN_4 | GPIO_PIN_5);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4 | GPIO_PIN_5);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2, GPIO_PIN_4 | GPIO_PIN_5);

    /* configure EXMC NE0 */
    gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_4);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL2, GPIO_PIN_4);

    lcd_timing_init_struct.syn_data_latency = EXMC_DATALAT_2_CLK;
    lcd_timing_init_struct.syn_clk_division = EXMC_SYN_CLOCK_RATIO_DISABLE;
    lcd_timing_init_struct.bus_latency = 2;
    lcd_timing_init_struct.asyn_data_setuptime = 15;
    lcd_timing_init_struct.asyn_address_holdtime = 2;
    lcd_timing_init_struct.asyn_address_setuptime = 8;

    lcd_init_struct.write_mode = EXMC_ASYN_WRITE;
    lcd_init_struct.asyn_wait = DISABLE;
    lcd_init_struct.nwait_signal = DISABLE;
    lcd_init_struct.memory_write = ENABLE;
    lcd_init_struct.nwait_config = EXMC_NWAIT_CONFIG_BEFORE;
    lcd_init_struct.wrap_burst_mode = DISABLE;
    lcd_init_struct.nwait_polarity = EXMC_NWAIT_POLARITY_LOW;
    lcd_init_struct.burst_mode = DISABLE;
    lcd_init_struct.databus_width = EXMC_NOR_DATABUS_WIDTH_16B;
    lcd_init_struct.memory_type = EXMC_MEMORY_TYPE_NOR;
    lcd_init_struct.address_data_mux = DISABLE;
    lcd_init_struct.read_write_timing = &lcd_timing_init_struct;

    exmc_norsram_init(&lcd_init_struct);
    exmc_norsram_enable();
}

/*!
    \brief      write data to the selected LCD register
    \param[in]  register_id: the selected register id
    \param[in]  value: the register value to be written
    \param[out] none
    \retval     none
*/

void lcd_register_write(uint16_t register_id,uint16_t value)
{
    *(__IO uint16_t *) (BANK0_LCD_C)= register_id;
    *(__IO uint16_t *) (BANK0_LCD_D)= value;
}

/*!
    \brief      read the value of LCD register
    \param[in]  register_id: the register id
    \param[out] none
    \retval     the register value
*/
uint16_t lcd_register_read(uint8_t register_id)
{
    uint16_t data;
    *(__IO uint16_t *) (BANK0_LCD_C)= register_id;
    data = *(__IO uint16_t *) (BANK0_LCD_D); 
    return  data;
}

/*!
    \brief      write command to LCD register
    \param[in]  value: the register value to be written
    \param[out] none
    \retval     none
*/
void lcd_command_write(uint16_t value)
{
    /* write 16-bit index, then write reg */
    *(__IO uint16_t *) (BANK0_LCD_C) = value;
}

/*!
    \brief      prepare to write to the LCD GRAM register(R22h)
    \param[in]  none
    \param[out] none
    \retval     none
*/
void lcd_gram_write_prepare(void)
{
    *(__IO uint16_t *) (BANK0_LCD_C) = 0x0022;
}

/*!
    \brief      write RGB code to the LCD GRAM register
    \param[in]  rgb_code: the pixel color in RGB mode (5-6-5)
    \param[out] none
    \retval     none
*/
void lcd_gram_write(uint16_t rgb_code)
{
    /* write 16-bit GRAM register */
    *(__IO uint16_t *) (BANK0_LCD_D) = rgb_code;
}

/*!
    \brief      read data from GRAM
    \param[in]  none
    \param[out] none
    \retval     GRAM value
*/
uint16_t lcd_gram_read(void)
{
  uint16_t data;
    
  /* write GRAM register (R22h) */
  *(__IO uint16_t *) (BANK0_LCD_C) = 0x0022;
  /* dummy read (invalid data) */
  data = *(__IO uint16_t *) (BANK0_LCD_D);

  data = *(__IO uint16_t *) (BANK0_LCD_D); 
  return data;
}

/*!
    \brief      initialize the LCD
    \param[in]  none
    \param[out] none
    \retval     none
*/
void lcd_init(void)
{
    __IO uint16_t i;

    /* read the LCD controller device code */
    device_code = lcd_register_read(0x0000);

    if(0x8989 == device_code){             // SSD1289
        lcd_register_write(0x0000,0x0001);
        lcd_register_write(0x0003,0xA8A4);
        lcd_register_write(0x000C,0x0000);
        lcd_register_write(0x000D,0x080C);
        lcd_register_write(0x000E,0x2B00);
        lcd_register_write(0x001E,0x00B0);
        lcd_register_write(0x0001,0x6B3F);
        lcd_register_write(0x0002,0x0600);
        lcd_register_write(0x0010,0x0000);
        lcd_register_write(0x0011,0x6078);
        lcd_register_write(0x0005,0x0000);
        lcd_register_write(0x0006,0x0000);
        lcd_register_write(0x0016,0xEF1C);
        lcd_register_write(0x0017,0x0003);
        lcd_register_write(0x0007,0x0233);
        lcd_register_write(0x000B,0x0000);
        lcd_register_write(0x000F,0x0000);
        lcd_register_write(0x0041,0x0000);
        lcd_register_write(0x0042,0x0000);
        lcd_register_write(0x0048,0x0000);
        lcd_register_write(0x0049,0x013F);
        lcd_register_write(0x004A,0x0000);
        lcd_register_write(0x004B,0x0000);
        lcd_register_write(0x0044,0xEF00);
        lcd_register_write(0x0045,0x0000);
        lcd_register_write(0x0046,0x013F);
        lcd_register_write(0x0030,0x0707);
        lcd_register_write(0x0031,0x0204);
        lcd_register_write(0x0032,0x0204);
        lcd_register_write(0x0033,0x0502);
        lcd_register_write(0x0034,0x0507);
        lcd_register_write(0x0035,0x0204);
        lcd_register_write(0x0036,0x0204);
        lcd_register_write(0x0037,0x0502);
        lcd_register_write(0x003A,0x0302);
        lcd_register_write(0x003B,0x0302);
        lcd_register_write(0x0023,0x0000);
        lcd_register_write(0x0024,0x0000);
        lcd_register_write(0x0025,0x8000);
        lcd_register_write(0x004e,0);
        lcd_register_write(0x004f,0);
    }else if((0x9320 == device_code) || (0x9300 == device_code)){    //ILI9320
        lcd_register_write(0x01,0x0100);         //driver output control
        lcd_register_write(0x02,0x0700);         //lcd driver waveform control
        lcd_register_write(0x03,0x1030);         //entry mode set for portrait GRAM writes

        lcd_register_write(0x04,0x0000);         //resizing control
        lcd_register_write(0x08,0x0202);         //display control 2
        lcd_register_write(0x09,0x0000);         //display control 3
        lcd_register_write(0x0a,0x0000);         //frame cycle control
        lcd_register_write(0x0c,(1<<0));         //extern display interface control 1
        lcd_register_write(0x0d,0x0000);         //frame maker position
        lcd_register_write(0x0f,0x0000);         //extern display interface control 2

        for(i=50000;i>0;i--);
        lcd_register_write(0x07,0x0101);         //display control
        for(i=50000;i>0;i--);

        lcd_register_write(0x10,(1<<12)|(0<<8)|(1<<7)|(1<<6)|(0<<4));        //power control 1
        lcd_register_write(0x11,0x0007);                                     //power control 2
        lcd_register_write(0x12,(1<<8)|(1<<4)|(0<<0));                       //power control 3
        lcd_register_write(0x13,0x0b00);                                     //power control 4
        lcd_register_write(0x29,0x0000);                                     //power control 7
        lcd_register_write(0x2b,(1<<14)|(1<<4));
        lcd_register_write(0x50,0);              //set controller x start
        lcd_register_write(0x51,239);            //set controller x end
        lcd_register_write(0x52,0);              //set controller y start
        lcd_register_write(0x53,319);            //set controller y end

        lcd_register_write(0x60,0x2700);         //driver output control, reverse vertical scan
        lcd_register_write(0x61,0x0001);         //driver output control
        lcd_register_write(0x6a,0x0000);         //vertical srcoll control

        lcd_register_write(0x80,0x0000);         //display position? partial display 1
        lcd_register_write(0x81,0x0000);         //ram address start? partial display 1
        lcd_register_write(0x82,0x0000);         //ram address end-partial display 1
        lcd_register_write(0x83,0x0000);         //display position? partial display 2
        lcd_register_write(0x84,0x0000);         //ram address start? partial display 2
        lcd_register_write(0x85,0x0000);         //ram address end? partial display 2

        lcd_register_write(0x90,(0<<7)|(16<<0)); //frame cycle control
        lcd_register_write(0x92,0x0000);         //panel interface control 2
        lcd_register_write(0x93,0x0001);         //panel interface control 3
        lcd_register_write(0x95,0x0110);         //frame cycle control
        lcd_register_write(0x97,(0<<8));
        lcd_register_write(0x98,0x0000);         //frame cycle control
        for(i=50000;i>0;i--);
        lcd_register_write(0x07,0x0173);
        for(i=50000;i>0;i--);

    }else{
        return;
    }
    
    for(i=50000;i>0;i--);
}

/*!
    \brief      initialize DMA used for LCD GRAM transfers
    \param[in]  none
    \param[out] none
    \retval     none
*/
void lcd_dma_init(void)
{
    dma_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(LCD_DMA, LCD_DMA_CHANNEL);
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.request = DMA_REQUEST_M2M;
    dma_init_struct.periph_addr = BANK0_LCD_D;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_init_struct.memory_addr = 0U;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_16BIT;
    dma_init_struct.number = 0U;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;

    dma_init(LCD_DMA, LCD_DMA_CHANNEL, &dma_init_struct);
    dma_memory_to_memory_enable(LCD_DMA, LCD_DMA_CHANNEL);
    dma_interrupt_enable(LCD_DMA, LCD_DMA_CHANNEL, DMA_INT_FTF | DMA_INT_ERR);
    dma_flag_clear(LCD_DMA, LCD_DMA_CHANNEL, DMA_INT_FLAG_GIF | DMA_INT_FLAG_FTF | DMA_INT_FLAG_ERR | DMA_INT_FLAG_HTF);
    nvic_irq_enable(LCD_DMA_IRQn, 1U, 0U);
}

/*!
    \brief      get LCD DMA transfer state
    \param[in]  none
    \param[out] none
    \retval     SET if DMA transfer is in progress, RESET otherwise
*/
FlagStatus lcd_dma_busy_get(void)
{
    return lcd_dma_busy;
}

/*!
    \brief      set the cursor of LCD
    \param[in]  x: the row-coordinate
    \param[in]  y: the column-coordinate
    \param[out] none
    \retval     none
*/
void lcd_cursor_set(uint16_t x,uint16_t y)
{
    uint16_t controller_x = x;
    uint16_t controller_y = y;

    if(0x8989 == device_code){             // SSD1289
        lcd_register_write(0x004e, controller_x);
        lcd_register_write(0x004f, controller_y);
    }else if((0x9320 == device_code) || (0x9300 == device_code)){    //ILI9320
        lcd_register_write(0x20, controller_x);
        lcd_register_write(0x21, controller_y);
    }
}

/*!
    \brief      clear the LCD screen to the specified color
    \param[in]  color: specified screen color
    \param[out] none
    \retval     none
*/
void lcd_clear(uint16_t color)
{
    uint32_t index=0;
    if(0x8989 == device_code){             // SSD1289
        lcd_cursor_set(0,0);
        /* prepare to write GRAM */
        lcd_gram_write_prepare();
        for(index=0; index<LCD_PIXEL_WIDTH*LCD_PIXEL_HEIGHT; index++){
            *(__IO uint16_t *) (BANK0_LCD_D) = color;
        }
    }else if((0x9320 == device_code) || (0x9300 == device_code)){    //ILI9320
        lcd_cursor_set(0,0);
        lcd_command_write(0x22);
        for(index=0; index<LCD_PIXEL_WIDTH*LCD_PIXEL_HEIGHT; index++){
            *(__IO uint16_t *) (BANK0_LCD_D) = color;
        }
    }
}

/*!
    \brief      set the point according to the specified position and color
    \param[in]  x: the row-coordinate
    \param[in]  y: the column-coordinate
    \param[in]  point: specified color of the point
    \param[out] none
    \retval     none
*/
void lcd_point_set(uint16_t x,uint16_t y,uint16_t point)
{
    if ((x >= LCD_PIXEL_WIDTH)||(y >= LCD_PIXEL_HEIGHT)){
        return;
    }
    if(0x8989 == device_code){             // SSD1289
        lcd_cursor_set(x,y);
        lcd_gram_write_prepare();
        lcd_gram_write(point);
    }else if((0x9320 == device_code) || (0x9300 == device_code)){    //ILI9320
        lcd_cursor_set(x, y);
        lcd_register_write(0x22, point);
    }
}

/*!
    \brief      get point GRAM according to the specified position
    \param[in]  x: the row-coordinate
    \param[in]  y: the column-coordinate
    \param[out] none
    \retval     GRAM value of point 
*/
uint16_t lcd_point_get(uint16_t x,uint16_t y)
{
    uint16_t data;
    
    if ((x >= LCD_PIXEL_WIDTH)||(y >= LCD_PIXEL_HEIGHT)){
        return 0;
    }
    
    lcd_cursor_set(x,y);
    data = lcd_gram_read();

    return  data;
}

/*!
    \brief      set window area
    \param[in]  start_x: the start position of row-coordinate
    \param[in]  start_y: the start position of column-coordinate
    \param[in]  end_x: the end position of row-coordinate
    \param[in]  end_y: the end position of column-coordinate
    \param[out] none
    \retval     none
*/
void lcd_windows_set(uint16_t start_x,uint16_t start_y,uint16_t end_x,uint16_t end_y)
{
    uint16_t controller_x_start = start_x;
    uint16_t controller_x_end = end_x;
    uint16_t controller_y_start = start_y;
    uint16_t controller_y_end = end_y;

    if(0x8989 == device_code){             // SSD1289
        lcd_register_write(0x0044, controller_x_start + (controller_x_end<<8));
        lcd_register_write(0x0045, controller_y_start);
        lcd_register_write(0x0046, controller_y_end);
        
        lcd_cursor_set(start_x, start_y);
        lcd_gram_write_prepare();
    }else if((0x9320 == device_code) || (0x9300 == device_code)){    //ILI9320
        lcd_register_write(0x0050, controller_x_start);
        lcd_register_write(0x0052, controller_y_start);
        lcd_register_write(0x0051, controller_x_end);
        lcd_register_write(0x0053, controller_y_end);

        lcd_cursor_set(start_x, start_y);
        lcd_gram_write_prepare();
    }
}

/*!
    \brief      draw a horizontal line on LCD screen
    \param[in]  x: the row-coordinate
    \param[in]  start_y: the start column-coordinate
    \param[in]  end_y: the end column-coordinate
    \param[in]  color: specified color of the point
    \param[in]  width: line width
    \param[out] none
    \retval     none
*/

void lcd_hline_draw(uint16_t x,uint16_t start_y,uint16_t end_y,uint16_t color,uint16_t width)
{
    uint16_t i, y;

    for (i = 0; i < width; i++) {
        uint16_t sx = x + i;

        for (y = start_y; y < end_y; y++) {
            lcd_point_set(sx, y, color);
        }
    }
}

/*!
    \brief      draw a vertical line on LCD screen
    \param[in]  start_x: the start column-coordinate
    \param[in]  end_x: the end column-coordinate
    \param[in]  y: the row-coordinate
    \param[in]  color: specified color of the point
    \param[in]  width: line width
    \param[out] none
    \retval     none
*/

void lcd_vline_draw(uint16_t start_x,uint16_t end_x,uint16_t y,uint16_t color,uint16_t width)
{
    uint16_t i, x;

    for (i = 0; i < width; i++) {
        uint16_t sy = y + i;

        for (x = start_x; x < end_x; x++) {
            lcd_point_set(x, sy, color);
        }
    }
}

/*!
    \brief      draw a rectangle according to the specified position and color
    \param[in]  start_x: the start position of row-coordinate
    \param[in]  start_y: the start position of column-coordinate
    \param[in]  end_x: the end position of row-coordinate
    \param[in]  end_y: the end position of column-coordinate
    \param[in]  point: specified color of the point
    \param[out] none
    \retval     none
*/
void lcd_rectangle_draw(uint16_t start_x,uint16_t start_y,uint16_t end_x,uint16_t end_y,uint16_t point)
{
    uint16_t x,y;
    x=start_x;
    y=start_y;
    /* draw four lines */
    for(x=start_x;x<end_x;x++){
        /* draw a point */
        lcd_point_set(x,y,point);
    }
    for(y=start_y;y<end_y;y++){
        lcd_point_set(x,y,point);
    }
    for(x=end_x;x>start_x;x--){
        lcd_point_set(x,y,point);
    }
    for(y=end_y;y>start_y;y--){
        lcd_point_set(x,y,point);
    }
}

/*!
    \brief      start an asynchronous picture transfer to LCD using DMA
    \param[in]  start_x: the start position of row-coordinate
    \param[in]  start_y: the start position of column-coordinate
    \param[in]  end_x: the end position of row-coordinate
    \param[in]  end_y: the end position of column-coordinate
    \param[in]  pic: the picture pointer
    \param[out] none
    \retval     none
*/
void lcd_picture_draw_dma(uint16_t start_x,uint16_t start_y,uint16_t end_x,uint16_t end_y,const uint16_t *pic)
{
    uint32_t total;

    total = (uint32_t)(end_x - start_x + 1U) * (uint32_t)(end_y - start_y + 1U);
    lcd_windows_set(start_x, start_y, end_x, end_y);

    dma_channel_disable(LCD_DMA, LCD_DMA_CHANNEL);
    dma_flag_clear(LCD_DMA, LCD_DMA_CHANNEL, DMA_INT_FLAG_GIF | DMA_INT_FLAG_FTF | DMA_INT_FLAG_ERR | DMA_INT_FLAG_HTF);
    dma_periph_address_config(LCD_DMA, LCD_DMA_CHANNEL, BANK0_LCD_D);
    dma_memory_address_config(LCD_DMA, LCD_DMA_CHANNEL, (uint32_t)pic);
    dma_transfer_number_config(LCD_DMA, LCD_DMA_CHANNEL, total);

    lcd_dma_busy = SET;
    dma_channel_enable(LCD_DMA, LCD_DMA_CHANNEL);
}

/*!
    \brief      handle LCD DMA transfer interrupt
    \param[in]  none
    \param[out] none
    \retval     none
*/
void lcd_dma_irq_handler(void)
{
    if(SET == dma_interrupt_flag_get(LCD_DMA, LCD_DMA_CHANNEL, DMA_INT_FLAG_ERR)) {
        dma_interrupt_flag_clear(LCD_DMA, LCD_DMA_CHANNEL, DMA_INT_FLAG_ERR);
        dma_channel_disable(LCD_DMA, LCD_DMA_CHANNEL);
        lcd_dma_busy = RESET;
        return;
    }

    if(SET == dma_interrupt_flag_get(LCD_DMA, LCD_DMA_CHANNEL, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(LCD_DMA, LCD_DMA_CHANNEL, DMA_INT_FLAG_FTF);
        dma_channel_disable(LCD_DMA, LCD_DMA_CHANNEL);
        lcd_dma_busy = RESET;
    }
}

/*!
    \brief      fill the specified color to a rectangle
    \param[in]  start_x: the start position of row-coordinate
    \param[in]  start_y: the start position of column-coordinate
    \param[in]  end_x: the end position of row-coordinate
    \param[in]  end_y: the end position of column-coordinate
    \param[in]  color: specified color
    \param[out] none
    \retval     none
*/
void lcd_rectangle_fill(uint16_t start_x,uint16_t start_y,uint16_t end_x,uint16_t end_y,uint16_t color)
{
    uint16_t x, y;
    x = start_x;
    y = start_y;

    for (x = start_x; x < end_x; x++) {
        for (y = start_y; y < end_y; y++) {
            lcd_point_set(x, y, color);
        }
    }
}

/*!
    \brief      draw a picture on LCD screen according to the specified position
    \param[in]  start_x: the start position of row-coordinate
    \param[in]  start_y: the start position of column-coordinate
    \param[in]  end_x: the end position of row-coordinate
    \param[in]  end_y: the end position of column-coordinate
    \param[in]  pic: the picture pointer
    \param[out] none
    \retval     none
*/
void lcd_picture_draw(uint16_t start_x,uint16_t start_y,uint16_t end_x,uint16_t end_y,uint16_t *pic)
{
    uint32_t width, height, row, col;
    uint16_t *picturepointer = pic;

    width = (uint32_t)(end_x - start_x + 1U);
    height = (uint32_t)(end_y - start_y + 1U);

    for(row = 0; row < height; row++) {
        lcd_windows_set(start_x, (uint16_t)(start_y + row), end_x, (uint16_t)(start_y + row));

        for(col = 0; col < width; col++) {
            *(__IO uint16_t *)(BANK0_LCD_D) = *picturepointer++;
        }
    }
}

/*!
    \brief      display a char on LCD screen according to the specified position
    \param[in]  x: the start position of row-coordinate
    \param[in]  y: the start position of column-coordinate
    \param[in]  c: the char
    \param[in]  char_color: the color of char
    \param[in]  c_format: the structure of char format
                  font: CHAR_FONT_8_16 or CHAR_FONT_16_24
                  direction: CHAR_DIRECTION_HORIZONTAL or CHAR_DIRECTION_VERTICAL
                  char_color: the color of char
                  bk_color: the color of background
    \param[out] none
    \retval     none
*/
void lcd_char_display(uint16_t x,uint16_t y,uint8_t c,char_format_struct c_format)
{
    (void)x;
    (void)y;
    (void)c;
    (void)c_format;
}
