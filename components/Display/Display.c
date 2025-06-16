//==================================================================================================
//
//	File Name		 : UI.c
//	CPU Type			: ESP32-C3
//	Project Name		: ESP32_C3_LCD_128Inch
//
//	Description		 : UI driver for ESP32-C3 with 1.28 inch LCD using LVGL
//
//	History			 : Ver.0.01		2024.11.27 V.Vu	 New
//
//==================================================================================================
//==================================================================================================
//	Compile Option
//==================================================================================================

//==================================================================================================
//	#pragma section
//==================================================================================================

//==================================================================================================
//	Local Compile Option
//==================================================================================================

//==================================================================================================
//	Header File
//==================================================================================================
#include <stdio.h>
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "CS816D.h"
#include "GC9A01.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "RamDef.h"
#include "lv_examples.h"
#include "demos/lv_demos.h"
#include <string.h>
//==================================================================================================
//	Local define
//==================================================================================================
#define U4_DISPLAY_HEIGHT	((U4)GC9A01_HEIGHT)
#define U4_DISPLAY_WIDTH	((U4)GC9A01_WIDTH)
//==================================================================================================
//	Local define I/O
//==================================================================================================

//==================================================================================================
//	Local Struct Template
//==================================================================================================

//==================================================================================================
//	Local RAM 
//==================================================================================================
static lv_display_t *pst_DisplayDriver = NULL;
U1 Drawn_Buffer_1[U4_DISPLAY_HEIGHT * U4_DISPLAY_WIDTH *2/10];
//==================================================================================================
//	Local ROM
//==================================================================================================
static const char *TAG = "UI";
//==================================================================================================
//	Local Function Prototype
//==================================================================================================
static void lvgl_tick_init(void);
static void lv_tick_task(void *arg);
static void display_Flush_Callback(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p);
static void create_InputDev(void);
//==================================================================================================
//	Source Code
//==================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			UI_Init
//	Name:			UI_Init
//	Function:		Initialize LVGL, display driver, and LCD buffer
//
//	Argument:		-
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		Initialize LVGL, display driver, buffer, and tick timer
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void UI_Init(void)
{
	// Init UI wifi scan follow Wifi scan state
	st_WifiSelected.u1_WifiPasswordValid_F = U1FALSE;			// Initialize WiFi password valid flag to FALSE

	lv_init();		// Initialize LVGL
	pst_DisplayDriver = lv_display_create(U4_DISPLAY_HEIGHT, U4_DISPLAY_WIDTH);		// Create display driver with LCD size
	lv_display_set_flush_cb(pst_DisplayDriver, display_Flush_Callback);				// Set flush callback for display driver

	// Set buffer for display driver
	lv_display_set_buffers(pst_DisplayDriver, Drawn_Buffer_1, NULL, sizeof(Drawn_Buffer_1), LV_DISPLAY_RENDER_MODE_PARTIAL);	
	lvgl_tick_init();			// Initialize LVGL tick timer
	create_InputDev();			// Create Input devices
	ui_init();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			??申?申??申?申??申?申yUI??申?申??申?申??申?申z
//	Name:			UI_Job
//	Function:		Handle UI tasks (should be called in main loop)
//
//	Argument:		-
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		 Call lv_timer_handler to update UI
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void UI_Job(void)
{
	// Handle LVGL tasks (update UI)
	vTaskDelay(5);
	lv_timer_handler();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			lvgl_tick_init
//	Name:			lvgl_tick_init
//	Function:		Initialize periodic timer for LVGL tick
//
//	Argument:		-
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		 Create timer to call lv_tick_task every 1ms
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void lvgl_tick_init(void)
{
	// Configure parameters for periodic timer
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &lv_tick_task,
		.name = "lvgl_tick"
	};

	esp_timer_handle_t periodic_timer;

	// Create periodic timer
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

	// Start timer with 1ms period
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000)); // 1000us = 1ms
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			lv_tick_task
//	Name:			lv_tick_task
//	Function:		Increase LVGL tick every 1ms
//
//	Argument:		arg - not used
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		 Callback for LVGL timer
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void lv_tick_task(void *arg)
{
	// Increase LVGL tick (1ms)
	lv_tick_inc(1);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			display_Flush_Callback
//	Name:			display_Flush_Callback
//	Function:		Send display area data from LVGL to LCD
//
//	Argument:		disp		- LVGL display driver
//					area		- area to update
//					color_p - color buffer
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		 Called by LVGL when display area needs update
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void display_Flush_Callback(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p)
{
	// Get coordinates of area to update
	U4 au4_x1 = area->x1;
	U4 au4_y1 = area->y1;
	U4 au4_x2 = area->x2;
	U4 au4_y2 = area->y2;
	U4 au4_size = (au4_x2 - au4_x1 + 1) * (au4_y2 - au4_y1 + 1);

	// Swap bytes in color_p buffer (assuming color_p is uint16_t array)
	uint16_t *buf = (uint16_t *)color_p;
	for (U4 i = 0; i < au4_size; i++) {
		uint16_t px = buf[i];
		buf[i] = (px >> 8) | (px << 8);
	}

	GC9A01_SetWindow(au4_x1, au4_y1, au4_x2, au4_y2); // Set drawing window on LCD
	lcd_data((uint8_t *)color_p, au4_size * sizeof(uint16_t)); // Send color data to LCD
	lv_disp_flush_ready(disp); // Notify LVGL that flushing is done
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			my_input_read
//	Name:			my_input_read
//	Function:		Read touch input from CS816D and update LVGL input data
//
//	Argument:		-
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		 Create a button at the center of the screen
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void my_input_read(lv_indev_t * indev, lv_indev_data_t * data)
{
	U1 u1_Pressed;
	u1_Pressed = u1_CS816D_ReadTouch(&st_TouchData);	// Read touch data from CS816D
	if(u1_Pressed == U1OK) {
		data->point.x = st_TouchData.u2_X;
		data->point.y = st_TouchData.u2_Y;
		data->state = LV_INDEV_STATE_PRESSED;
	} else {
		data->state = LV_INDEV_STATE_RELEASED;
	}
}
static void create_InputDev(void)
{
	lv_indev_t * indev = lv_indev_create();        /* Create input device connected to Default Display. */
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);   /* Touch pad is a pointer-like device. */
	lv_indev_set_read_cb(indev, my_input_read);    /* Set driver function. */
}