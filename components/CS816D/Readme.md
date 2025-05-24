# CS816D Component

## Overview

**CS816D** is a touch driver component used in the **ESP-32_LCD 1.28 inch** project. It is responsible for handling capacitive touch input from the round 1.28-inch TFT LCD screen.

The CS816D communicates with the ESP32 microcontroller over the I2C interface, enabling precise touch detection and gesture recognition on the display.

## Purpose in the Project

- **Touch Input Handling**: Detects and reports touch events (e.g., press, release, drag) from the TFT LCD.
- **I2C Communication**: Interfaces with the ESP32 via the I2C protocol for efficient data transfer.
- **User Interaction**: Enables interactive features such as buttons, sliders, and gestures on the display.

## Features

- Capacitive touch support
- I2C address: (please fill in the correct address if known)
- Supports single-touch or multi-touch depending on configuration
- Compatible with 1.28-inch round TFT LCD used in the project

## Integration Notes

- Ensure proper I2C configuration on ESP32 (SDA, SCL pins and pull-up resistors).
- The CS816D driver should be initialized before the main application loop.
- Touch event data can be read periodically or through interrupt-based handling.

## Project Context

This component is part of the **ESP-32_LCD 1.28 inch** project, which features a round TFT display with capacitive touch capability. The CS816D allows the ESP32 to receive touch input, making the display interactive and user-friendly.

