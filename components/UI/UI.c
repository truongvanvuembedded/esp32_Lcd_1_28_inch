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
#include "UI.h"
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
extern DMA_ATTR uint16_t u2_ScreenBuff[U4_DISPLAY_HEIGHT * U4_DISPLAY_WIDTH];
static lv_display_t *pst_DisplayDriver = NULL;
U1 Drawn_Buffer_1[U4_DISPLAY_HEIGHT * U4_DISPLAY_WIDTH *2/10];
U1 Drawn_Buffer_2[U4_DISPLAY_HEIGHT * U4_DISPLAY_WIDTH *2/10];
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
static void create_ui(void);
static void create_InputDev(void);
//==================================================================================================
//	Source Code
//==================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			UI�z
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
	lv_init();		// Initialize LVGL
	pst_DisplayDriver = lv_display_create(U4_DISPLAY_HEIGHT, U4_DISPLAY_WIDTH);		// Create display driver with LCD size
	lv_display_set_flush_cb(pst_DisplayDriver, display_Flush_Callback);				// Set flush callback for display driver

	// Set buffer for display driver
	lv_display_set_buffers(pst_DisplayDriver, Drawn_Buffer_1, Drawn_Buffer_2, sizeof(Drawn_Buffer_1), LV_DISPLAY_RENDER_MODE_PARTIAL);	
	lvgl_tick_init();			// Initialize LVGL tick timer
	create_ui();				// Create basic UI (button)
	create_InputDev();			// Create Input devices
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			�yUI�z
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
	lv_timer_handler();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			�yUI�z
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
//	Tag:			�yUI�z
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
//	Tag:			�yUI�z
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
	U4 au4_size;

	GC9A01_SetWindow(au4_x1, au4_y1, au4_x2, au4_y2);									// Set drawing window on LCD
	au4_size = (au4_x2 - au4_x1 + 1) * (au4_y2 - au4_y1 + 1) * sizeof(uint16_t);		// Calculate size of area to send
	lcd_data((uint8_t *)color_p, au4_size);												// Send color data to LCD
	lv_disp_flush_ready(disp);															// Notify LVGL that flushing is done
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			�yUI�z
//	Name:			create_ui
//	Function:		Create basic user interface (button)
//
//	Argument:		-
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		 Create a button at the center of the screen
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void button_event_cb(lv_event_t * e)
{

}
static void create_ui(void)
{
	//lv_obj_t * button = lv_button_create(lv_screen_active());
	//lv_obj_center(button);
	//lv_obj_set_size(button, 100, 50);
	//lv_obj_add_event_cb(button, button_event_cb, LV_EVENT_CLICKED, NULL);

	//lv_obj_t * label = lv_label_create(button);
	//lv_label_set_text(label, "Button");
	//lv_obj_center(label);

	//lv_example_get_started_1();	// Create a simple "Hello World" label
	lv_example_get_started_2();	// Create a button with a label and react on click event
	//lv_example_get_started_3();	// Create styles from scratch for buttons
	//lv_example_get_started_4();	// Create a slider and write its value on a label

	// lv_demo_benchmark();	// Create a demo benchmark screen

}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			�yUI�z
//	Name:			create_InputDev
//	Function:		Create input devices
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