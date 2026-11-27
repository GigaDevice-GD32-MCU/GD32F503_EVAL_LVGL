# LVGL Ported to GD32F503V EVAL

## Overview

This project ports `LVGL` to the `GD32F503V EVAL` evaluation board for GUI demonstrations.

The board features a `GD32F503VG` microcontroller based on the `ARM Cortex-M33` core,
along with a `240 x 320` TFT display and a resistive touch panel.
The project uses portrait orientation with the `RGB565` pixel format and 16-bit color depth.

Project toolchains include `Keil MDK-ARM (ARM Compiler 6)`, `IAR`, and `GD32EmbeddedBuilder`.

## Benchmark

The following benchmark results were obtained using LVGL 9.6.0 with DMA acceleration and double buffering.

| Name                      | Avg. CPU | Avg. FPS | Avg. time | render time | flush time |
| :------------------------ | -------: | -------: | --------: | ----------: | ---------: |
| Empty screen              | 46%      | 32       | 10        | 10          | <1         |
| Moving wallpaper          | 69%      | 33       | 20        | 20          | <1         |
| Single rectangle          | 12%      | 33       | 1         | 1           | <1         |
| Multiple rectangles       | 47%      | 33       | 13        | 13          | <1         |
| Multiple RGB images       | 16%      | 33       | 3         | 3           | <1         |
| Multiple ARGB images      | 17%      | 33       | 4         | 4           | <1         |
| Rotated ARGB images       | 84%      | 31       | 29        | 29          | <1         |
| Multiple labels           | 34%      | 33       | 7         | 7           | <1         |
| Screen sized text         | 96%      | 10       | 95        | 95          | <1         |
| Multiple arcs             | 17%      | 33       | 2         | 2           | <1         |
| Containers                | 21%      | 33       | 9         | 9           | <1         |
| Containers with overlay   | 65%      | 33       | 22        | 22          | <1         |
| Containers with opa       | 34%      | 33       | 10        | 10          | <1         |
| Containers with opa_layer | 29%      | 33       | 12        | 12          | <1         |
| Containers with scrolling | 85%      | 32       | 27        | 27          | <1         |
| Widgets demo              | 99%      | 23       | 39        | 39          | <1         |
| All scenes avg.           | 48%      | 30       | 18        | 18          | <1         |
 
## Specification 

- **MCU:** GD32F503VG (Cortex-M33, 252MHz)
- **RAM:** 128KB internal SRAM
- **Flash:** 1024KB internal
- **GPU:** None
- **Resolution:** 240*320
- **Display Size:** 3.2 inch
- **Interface:** 8080
- **Color Depth:** 16-bit RGB565
- **Technology:** a-Si TFT
- **DPI:** 130 px/inch
- **Touch Pad:** Resistive touch (SPI)

## Getting Started

The following steps describe a general workflow. Confirm the project paths, software versions,
power supply requirements, and jumper settings against the actual project and the board manual.
This project currently supports three integrated development environments (IDEs): Keil MDK, IAR, and GD32EmbeddedBuilder. This document uses GD32EmbeddedBuilder to demonstrate how to build, download, and debug the project.

### Hardware Setup

1. Confirm that the board model is `GD32F503V EVAL`.
2. Check the LCD display and resistive touch panel connections according to the board manual.
3. Connect the on-board JLINK debug interface to your PC, and configure the power supply and jumpers as required.

### Software Setup

1. Install `GD32EmbeddedBuilder` for your operating system. Refer to the GD32EmbeddedBuilder software requirements for details.
2. Install the appropriate J-Link driver software and confirm that your PC recognizes the debugger.

### Build the Project

1. Clone this repository.
  ```shell
  git clone "Repository URL"
  ```
2. Open `GD32EmbeddedBuilder` and click **Browse...** to select a local directory as your workspace.
3. Import the project by selecting **File > Import...** and choosing the project directory (`./Template/EB_project`).
4. Build the project by right-clicking the project in the project explorer and selecting **Build Project**.
5. To download and debug the project, right-click the project in the project explorer and select
	**Debug As > Debug Configurations... > GDB General Debugging**. Configure the settings for your setup,
	then click **Debug** to download the firmware and start debugging.

### Debugging

Start a debug session using your IDE and JLINK to set breakpoints, step through code, and inspect variables.
If display or touch issues occur, first check peripheral initialization, display refresh, and touch input handling.
The available debug options depend on your IDE and project configuration.

## Third-Party Components

| Category | In Use | Component | Version | License |
| --- | --- | --- | --- | --- |
| GUI | Yes | `LVGL` | `V9.6.0` | `MIT` |

When adding a third-party library, update this table and retain its license text and copyright notices.

## Contribution and Support

If you find any issues with the development board feel free to open an Issue in this repository. For LVGL related issues (features, bugs, etc) please use the main [lvgl repository](https://github.com/lvgl/lvgl).

If you found a bug and found a solution too please send a Pull request. If you are new to Pull requests refer to [Our Guide](https://docs.lvgl.io/master/CONTRIBUTING.html#pull-request) to learn the basics.
