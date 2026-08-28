# LVGL V8.3 ported to the GD32F503V EVAL

This project ports `LVGL V8.3.11` to the `GD32F503V-EVAL` platform for GUI demonstrations.

## Hardware Information

The `GD32F503V-EVAL-V1.1` board is based on:

- `GD32F503xG` microcontroller (`ARM Cortex-M33` core)
- `1024 KB` on-chip Flash memory and `128 KB` on-chip SRAM
- `240 x 320` TFT display with resistive touch panel (`RGB565` 16-bit color, portrait orientation)
- LCD controller: auto-detected; supports `ILI9320`/`ILI9325` and `SSD1289`
- LCD interface: `EXMC` asynchronous 16-bit parallel bus (NOR/SRAM Bank 0)
- LCD transfer: `DMA1 Channel 0` transfers to LCD GRAM
- Resistive touch panel connected through `SPI2`
- Three user LEDs on `PC7`, `PC8`, and `PC9`
- Three user buttons: Wakeup (`PA0`), Tamper (`PC13`), and User (`PA1`)
- `USART0` interface on `PA9`/`PA10` for serial communication

## Project Information

- GUI framework: `LVGL V8.3.11`
- Toolchain: `Keil MDK-ARM / IAR / GD32EmbeddedBuilder`
- Target board: `GD32F503V-EVAL-V1.1`
- Display configuration: `240 x 320 / RGB565 16-bit color / portrait`

## Third-Party Components

| Category | In use | Component | Version | License |
| -------- | ------ | --------- | ------- | ------- |
| GUI      | `Yes`  | `LVGL`    | `V8.3.11` | `MIT` |

> When adding a third-party library, update this table and retain its license text and copyright notice.
