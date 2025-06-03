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
static lv_obj_t * WifiList;
U1 Drawn_Buffer_1[U4_DISPLAY_HEIGHT * U4_DISPLAY_WIDTH *2/10];
static U1 u1_UIWifiScanShown_F;
static lv_obj_t* wifi_body;
static lv_obj_t* wifi_head;
static lv_obj_t* WifiSwitch;
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
static void UI_WifiScan(void);
static void ui_WifiList_AddItems(lv_obj_t* list);
static void ui_WifiList_ClearItems(lv_obj_t* list);
static void ui_WifiList_load(lv_obj_t* list);
static void WifiList_UI(void);
static void SwitchEventCallback(lv_event_t * e);
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
	u1_UIWifiScanState = WIFI_SCAN_OFF;			// Initialize UI WiFi scan state to OFF
	u1_UIWifiScanState_Last = WIFI_SCAN_OFF;	// Initialize last UI WiFi scan state to OFF
	st_WifiSelected.u1_WifiPasswordValid_F = U1FALSE;			// Initialize WiFi password valid flag to FALSE

	lv_init();		// Initialize LVGL
	pst_DisplayDriver = lv_display_create(U4_DISPLAY_HEIGHT, U4_DISPLAY_WIDTH);		// Create display driver with LCD size
	lv_display_set_flush_cb(pst_DisplayDriver, display_Flush_Callback);				// Set flush callback for display driver

	// Set buffer for display driver
	lv_display_set_buffers(pst_DisplayDriver, Drawn_Buffer_1, NULL, sizeof(Drawn_Buffer_1), LV_DISPLAY_RENDER_MODE_PARTIAL);	
	lvgl_tick_init();			// Initialize LVGL tick timer
	create_InputDev();			// Create Input devices
	create_ui();				// Create basic UI (button)
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			?��?��?��yUI?��?��?��z
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
	UI_WifiScan();	// Handle WiFi scan state changes
	// Handle LVGL tasks (update UI)
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
//	Tag:			UI_WifiScan
//	Name:			UI_WifiScan
//	Function:		Handle WiFi scan state changes
//
//	Argument:		
//
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void UI_WifiScan(void)
{
	if(u1_UIWifiScanState != u1_UIWifiScanState_Last) 
	{
		if(u1_UIWifiScanState == WIFI_SCAN_ON) 
		{
			ui_WifiList_load(wifi_body);
			u1_UIWifiScanShown_F = U1FALSE;  // reset flag
		} 
		else 
		{
			ui_WifiList_ClearItems(wifi_body);
		}
		u1_UIWifiScanState_Last = u1_UIWifiScanState;
	}

	// Khi scan xong và chưa cập nhật UI
	if(u1_UIWifiScanState == WIFI_SCAN_ON && u1_WifiScanDone_F == U1TRUE && u1_UIWifiScanShown_F == U1FALSE)
	{
		ESP_LOGI(TAG, "WiFi scan done, updating UI list");
		ui_WifiList_AddItems(wifi_body);
		u1_UIWifiScanShown_F = U1TRUE; // đánh dấu đã hiển thị
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			
//	Name:			create_ui
//	Function:		Create user interface
//
//	Argument:		-
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		 Create a button at the center of the screen
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void SwitchEventCallback(lv_event_t * e)
{
	// Just set a flag, do not handle list here
	lv_obj_t * sw = lv_event_get_target(e); // Get the switch object
	if(lv_obj_has_state(sw, LV_STATE_CHECKED)) {
		u1_WifiScanState = WIFI_SCAN_ON; // Set WiFi scan state to ON
		u1_UIWifiScanState = WIFI_SCAN_ON; // Set UI WiFi scan state to ON
	} else {
		u1_WifiScanState = WIFI_SCAN_OFF; // Set WiFi scan state to OFF
		u1_UIWifiScanState = WIFI_SCAN_OFF; // Set UI WiFi scan state to OFF
	}
}
/*
 * NOTE: lv_obj_clean() will remove and DELETE all children of the object,
 * but the mbox itself is still valid. However, after cleaning, you must not
 * use any pointer to the old children. Also, lv_msgbox_add_* APIs expect
 * the mbox to be a valid msgbox object.
 *
 * If you want to update the content of the msgbox, you should NOT call lv_obj_clean()
 * on the msgbox itself, because this will remove the msgbox's internal structure,
 * and the msgbox will no longer be a valid msgbox object.
 *
 * Instead, you should close the old msgbox and create a new one, or use
 * lv_msgbox_add_text()/lv_msgbox_add_title() to update the content.
 */

// Create a timer to check connection status and update UI
static void wifi_connect_status_cb(lv_timer_t * timer)
{
	lv_obj_t * mbox = (lv_obj_t *)lv_timer_get_user_data(timer);
	if (u1_WifiConnected_F == U1TRUE && mbox) {
		// Clear spinner (remove all children except msgbox internal structure)
		uint32_t child_cnt = lv_obj_get_child_cnt(mbox);
		for (uint32_t i = 0; i < child_cnt; ) {
			lv_obj_t * child = lv_obj_get_child(mbox, i);
			// Only delete spinner, not msgbox internal objects (like close button)
			if (lv_obj_check_type(child, &lv_spinner_class)) {
				lv_obj_del(child);
				// Do not increment i, as child list is updated
				child_cnt--;
			}
			else if (lv_obj_check_type(child, &lv_label_class))
			{
				lv_obj_del(child);
				child_cnt--;
			}
			 else {
				i++;
			}
		}
		// Instead of cleaning the mbox, just update its text and title
		lv_msgbox_add_title(mbox, "Success");
		lv_msgbox_add_text(mbox, "\nConnect Successfully!");
		lv_timer_del(timer);
	}
}

static void ConfirmButtonEventCallback(lv_event_t* e)
{
	const char* password = (const char*)lv_event_get_user_data(e); // Get password string from user data
	ESP_LOGI(TAG, "Password pointer: %p, Password string: %s, Length: %d", password, password ? password : "(null)", password ? lv_strlen(password) : -1);
	if (password == NULL || lv_strlen(password) < 8) 
	{
		st_WifiSelected.u1_WifiPasswordValid_F = U1FALSE; // Set password valid flag to FALSE
		lv_obj_t * mbox1 = lv_msgbox_create(NULL);
		lv_obj_clear_flag(mbox1, LV_OBJ_FLAG_SCROLLABLE); // Clear scrollable flag
		lv_obj_set_width(mbox1, LV_PCT(80)); // Set width to 80% of parent
		lv_obj_set_height(mbox1, LV_PCT(50)); // Set height to 30% of parent
		lv_msgbox_add_title(mbox1, "Warning");
		lv_msgbox_add_text(mbox1, "Password must be at least 8 characters long.");
		lv_msgbox_add_close_button(mbox1);
	}
	else 
	{
		// Copy password vào mảng u1_password (giả sử mảng đủ lớn)
		lv_strcpy((char*)st_WifiSelected.u1_password, password);
		st_WifiSelected.u1_WifiPasswordValid_F = U1TRUE; // Set password valid flag to TRUE
		lv_obj_t * mbox1 = lv_msgbox_create(NULL);
		lv_obj_set_width(mbox1, LV_PCT(80)); // Set width to 80% of parent
		lv_obj_set_height(mbox1, LV_PCT(50)); // Set height to 30% of parent
		lv_msgbox_add_title(mbox1, "Connecting to WiFi");

		// Show spinner and "Connecting..." text initially
		lv_obj_t * spinner = lv_spinner_create(mbox1);
		lv_obj_set_size(spinner, 40, 40); // Set spinner size as needed
		lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 0); // Center the spinner in the message box
		lv_spinner_set_anim_params(spinner, 1000, 60); // Set animation time and arc length
		lv_msgbox_add_text(mbox1, "\nConnecting...");

		// create callback to handle WiFi connection
		lv_timer_create(wifi_connect_status_cb, 200, mbox1); // Check every 200ms
		lv_msgbox_add_close_button(mbox1);
	}
}
static void turnbackWifiScan(lv_event_t * e)
{
	lv_obj_t* Screen = lv_screen_active(); // Get the current screen
	lv_obj_clean(Screen); // Clear the current screen
	// Recreate the WiFi list UI
	WifiList_UI();
	//	Change the swtich button state to ON
	lv_obj_add_state(WifiSwitch, LV_STATE_CHECKED);
	u1_UIWifiScanShown_F = U1FALSE; // Reset the flag to indicate UI not shown yet
}
static void WifibuttonEventCallback(lv_event_t * e) 
{
	lv_obj_t* PassWordBody = NULL; // Declare input area pointer
	lv_obj_t* PasswordInputParent = lv_screen_active(); // Declare parent container for password input
	char* ssid = NULL;

		// Get the WiFi SSID from the button label
		ssid = (char *)lv_event_get_user_data(e); // Get the SSID from event user data
		if (ssid)
		{
			ESP_LOGI(TAG, "Selected WiFi SSID: %s", ssid);
			lv_strcpy((char *)st_WifiSelected.u1_ssid, ssid); // Copy SSID to selected WiFi struct
		}

		lv_obj_clean(PasswordInputParent); // Clear the current screen
		lv_obj_set_flex_flow(PasswordInputParent, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(PasswordInputParent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		
		// Create a container (PasswordHeader) for back button and label
		lv_obj_t* PasswordHeader = lv_obj_create(PasswordInputParent);
		lv_obj_set_style_pad_all(PasswordHeader, 0, LV_PART_MAIN);
		lv_obj_set_style_border_width(PasswordHeader, 0, 0);
		lv_obj_set_width(PasswordHeader, LV_PCT(100));
		lv_obj_set_height(PasswordHeader, LV_PCT(20));
		lv_obj_set_flex_flow(PasswordHeader, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(PasswordHeader, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_clear_flag(PasswordHeader, LV_OBJ_FLAG_SCROLLABLE);

		// Create a fixed-size back button
		lv_obj_t* back_btn = lv_button_create(PasswordHeader);
		lv_obj_clear_flag(back_btn, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_t* back_icon = lv_label_create(back_btn);
		lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
		lv_obj_add_event_cb(back_btn, turnbackWifiScan, LV_EVENT_CLICKED, NULL); // Link button to return event
		
		// Create a container for password input
		PassWordBody = lv_obj_create(PasswordInputParent);
		lv_obj_set_style_pad_all(PassWordBody, 0, LV_PART_MAIN);
		lv_obj_clear_flag(PassWordBody, LV_OBJ_FLAG_SCROLLABLE); // Ngăn scroll phần header
		lv_obj_set_flex_flow(PassWordBody, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(PassWordBody, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
		lv_obj_set_width(PassWordBody, LV_PCT(100));
		lv_obj_set_height(PassWordBody, LV_PCT(80));
		
		// Create a label
		lv_obj_t* label = lv_label_create(PassWordBody);
		lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN);
		lv_obj_set_width(label, LV_PCT(100)); // Set width to 100% of parent
		lv_obj_set_height(label, LV_PCT(5)); // Set height to 20% of parent
		lv_label_set_text(label, ssid);
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

		// Create text area for password input
		lv_obj_t* InputArea = lv_obj_create(PassWordBody);
		lv_obj_set_style_pad_all(InputArea, 0, LV_PART_MAIN);
		lv_obj_clear_flag(InputArea, LV_OBJ_FLAG_SCROLLABLE); // Ngăn scroll phần header
		lv_obj_set_width(InputArea, LV_PCT(100)); // Set width to 100% of parent
		lv_obj_set_height(InputArea, LV_PCT(15)); // Set height to 20% of parent
		lv_obj_set_flex_flow(InputArea, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(InputArea, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
		lv_obj_set_style_border_width(InputArea, 0, 0); // Chỉ ẩn viền mà giữ lại style khác

		lv_obj_t* PassWordTextArea = lv_textarea_create(InputArea);
		lv_obj_set_style_pad_all(PassWordTextArea, 0, LV_PART_MAIN);
		lv_obj_set_width(PassWordTextArea, LV_PCT(70)); // Set width to 100% of parent
		lv_obj_set_height(PassWordTextArea, LV_PCT(100)); // Set height to 20% of parent

		lv_obj_t* confirmButton = lv_button_create(InputArea);
		lv_obj_set_width(confirmButton, LV_PCT(20));   // Set width to 20% of parent
		lv_obj_set_height(confirmButton, LV_PCT(100)); // Set height to 100%
		lv_obj_set_style_pad_all(confirmButton, 0, LV_PART_MAIN);

		// Tạo label với biểu tượng OK
		lv_obj_t* icon = lv_label_create(confirmButton);
		lv_label_set_text(icon, LV_SYMBOL_OK); // Gán biểu tượng OK
		lv_obj_center(icon); // Căn giữa trong button

		// Không cần set size cho label icon
		// lv_obj_set_size(icon, lv_obj_get_width(confirmButton), lv_obj_get_height(confirmButton)); // Nên bỏ


		lv_obj_add_event_cb(confirmButton, ConfirmButtonEventCallback, LV_EVENT_CLICKED, (void *)lv_textarea_get_text(PassWordTextArea)); // Link button to confirm event
		
		// Create a keyboard for password input
		lv_obj_t* keyboard = lv_keyboard_create(PassWordBody);
		lv_obj_set_style_pad_all(keyboard, 0, LV_PART_MAIN);
		lv_obj_set_width(keyboard, LV_PCT(100)); // Set keyboard width to 100% of parent
		lv_obj_set_height(keyboard, LV_PCT(80)); // Set keyboard height to 30% of parent
		lv_keyboard_set_textarea(keyboard, PassWordTextArea); // Link keyboard to text area
}
// Helper function to add WiFi items to the list
static void ui_WifiList_AddItems(lv_obj_t* parent)
{
	if(parent) {
		lv_obj_clean(parent); // Clear existing items
		
		WifiList = lv_list_create(parent);
		lv_obj_set_style_pad_all(WifiList, 0, LV_PART_MAIN);
		lv_obj_set_width(WifiList, LV_PCT(100));
		lv_obj_set_height(WifiList, LV_PCT(100));
		lv_obj_set_flex_flow(WifiList, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(WifiList, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		// Example: add some dummy WiFi networks
		for (U1 au1_ForC = 0; au1_ForC < u2_WifiNum; au1_ForC++)
		{
			lv_obj_t* WifiButton = lv_list_add_button(WifiList, LV_SYMBOL_WIFI, (const char *)st_WifiInfo[au1_ForC].u1_ssid);
			lv_obj_set_flex_align(WifiButton, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
			lv_obj_add_event_cb(WifiButton, WifibuttonEventCallback, LV_EVENT_CLICKED, (void *)st_WifiInfo[au1_ForC].u1_ssid);
		}
	}
}

// Helper function to clear all WiFi items from the list
static void ui_WifiList_ClearItems(lv_obj_t* parent)
{
	if(parent) {
		lv_obj_clean(parent);
	}
}
static void ui_WifiList_load(lv_obj_t* parent)
{
	if(parent) {
		lv_obj_clean(parent); // Clear existing items

		// Create a container to center the spinner
		lv_obj_t * cont = lv_obj_create(parent);
		lv_obj_set_size(cont, lv_obj_get_width(parent), lv_obj_get_height(parent));
		lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

		// Create a spinner inside the container
		lv_obj_t * spinner = lv_spinner_create(cont);
		lv_obj_set_size(spinner, LV_PCT(50), LV_PCT(50));
		lv_spinner_set_anim_params(spinner, 5000, 200);
	}
}
static void WifiList_UI(void)
{
	lv_obj_t* screen = lv_screen_active();
	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE); // Ngăn scroll phần header

	// ---------- HEADER ----------
	wifi_head = lv_obj_create(screen);
	lv_obj_set_flex_flow(wifi_head, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(wifi_head, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_height(wifi_head, LV_PCT(20));
	lv_obj_set_width(wifi_head, LV_PCT(100));
	lv_obj_clear_flag(wifi_head, LV_OBJ_FLAG_SCROLLABLE); // Ngăn scroll phần header

	lv_obj_t * wifi_icon = lv_label_create(wifi_head);
	lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
	lv_obj_t* wifiLabel = lv_label_create(wifi_head);
	lv_label_set_text(wifiLabel, "WIFI");

	WifiSwitch = lv_switch_create(wifi_head);
	lv_obj_add_event_cb(WifiSwitch, SwitchEventCallback, LV_EVENT_VALUE_CHANGED, NULL);

	// ---------- BODY ----------
	wifi_body = lv_obj_create(screen);
	lv_obj_clear_flag(wifi_body, LV_OBJ_FLAG_SCROLLABLE); // Ngăn scroll 
	lv_obj_set_flex_flow(wifi_body, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(wifi_body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_width(wifi_body, LV_PCT(100));
	lv_obj_set_height(wifi_body, LV_PCT(100));

	// Do not add items here, handle in UI_Job
}
static void create_ui(void)
{
	WifiList_UI();	// Create a WiFi list UI
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