# UI Component

## Overview

The **UI** component is responsible for managing and rendering the graphical user interface (GUI) for the **ESP-32_LCD 1.28 inch** project. It includes all image assets and related logic required to display icons, backgrounds, and other visual elements on the screen.

This component works in conjunction with the **GC9A01** LCD driver and optionally a graphics library such as **LVGL**, or custom framebuffer rendering code.

## Purpose in the Project

- Stores and manages image assets used in the UI
- Handles drawing of interface elements to the screen
- Provides a modular structure for extending visual components

## Features

- Supports static image rendering (icons, logos, backgrounds)
- Optimized image format for embedded use (e.g., RGB565)
- Can integrate with touch input logic from the **CS816D** component
- Helps create interactive and visually rich displays

