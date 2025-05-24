# GC9A01 Component

## Overview

**GC9A01** is a display driver IC used to control the 1.28-inch round TFT LCD in the **ESP-32_LCD 1.28 inch** project. It communicates with the ESP32 microcontroller via the SPI interface and is responsible for rendering all graphical content on the screen.

## Features

- **Resolution**: 240 x 240 pixels
- **Display Type**: Round TFT LCD
- **Interface**: SPI (Serial Peripheral Interface)
- **Color Depth**: 65K/262K colors
- **Controller**: GC9A01
- **Display Size**: 1.28 inches (circular)

## Purpose in the Project

- Acts as the primary display interface in the ESP32 system
- Renders graphics, text, and UI elements
- Compatible with graphics libraries such as [LVGL](https://lvgl.io/) or custom framebuffers

## Integration Notes

- **SPI Configuration**: Connect MOSI, SCK, CS, DC, RESET, and optionally BL (Backlight)
- **Recommended SPI Clock**: Up to 40 MHz
- **DMA Support**: Can be used for faster rendering (optional)

## Usage

You can use this driver with:
- [ESP-IDF SPI Master Driver](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html)
- [LVGL graphics library](https://lvgl.io/)
- Or custom framebuffer-based rendering code

Typical initialization sequence includes:
- Hardware reset
- Command set initialization
- Setting rotation and color mode
- Clearing the screen before drawing


