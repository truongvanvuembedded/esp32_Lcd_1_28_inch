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

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"

#include "CS816D.h"
#include "GC9A01.h"
#include "Display.h"
#include "Define.h"
#ifndef REAL_RAM
#define REAL_RAM
#include "RamDef.h"
#endif
#include "Wifi_Scan.h"
#include "HTTP_Client.h"
#include <esp_sntp.h>
#include "lvgl.h"
#include "ui.h"
//==================================================================================================
//	Local define
//==================================================================================================

//==================================================================================================
//	Local define I/O
//==================================================================================================

//==================================================================================================
//	Local Struct Template
//==================================================================================================

//==================================================================================================
//	Local RAM 
//==================================================================================================
static time_t now;
static struct tm timeinfo;
//==================================================================================================
//	Local ROM
//==================================================================================================
//static const char *TAG = "main.c";
//==================================================================================================
//	Local Function Prototype
//==================================================================================================
static void update_TimeData(struct tm timeinfo);
//==================================================================================================
//	Source Code
//==================================================================================================

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	update_TimeData
//	Function:	Update system time data based on received information
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Processes and updates the system's time data using the latest received values.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void update_TimeData(struct tm timeinfo)
{
	st_DateTimeData.u1_Hour = timeinfo.tm_hour;
	st_DateTimeData.u1_Minute = timeinfo.tm_min;
	st_DateTimeData.u1_DayOfWeek = timeinfo.tm_wday;
	st_DateTimeData.u1_Day = timeinfo.tm_mday;
	st_DateTimeData.u1_Month = timeinfo.tm_mon + 1; // tm_mon is 0-based
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			main
//	Name:			app_main
//	Function:		Main function for ESP32-C3 application
//
//	Argument:		-
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Rtc_Task(void* pvparameter)
{
	while (1)
	{
		if (st_WeatherData.u1_GetWeatherDataSuccess_F == U1TRUE)
		{
			st_WeatherData.u1_GetWeatherDataSuccess_F = U1FALSE;

		}
		if (st_DateTimeData.u1_SynTimeDataSuccess_F == U1TRUE)
		{
			setenv("TZ", "ICT-7", 1); // ICT = Indochina Time (UTC+7)
			tzset();
			time(&now);
			localtime_r(&now, &timeinfo);
			update_TimeData(timeinfo);
			// Save previous values before updating
			// Update LVGL subjects only if value changed
			if (st_DateTimeData.u1_Hour != st_DateTimeData_Before.u1_Hour) {
				lv_subject_set_int(&subj_hour, st_DateTimeData.u1_Hour);
			}
			if (st_DateTimeData.u1_Minute != st_DateTimeData_Before.u1_Minute) {
				lv_subject_set_int(&subj_minute, st_DateTimeData.u1_Minute);
			}
			if (st_DateTimeData.u1_Second != st_DateTimeData_Before.u1_Second) {
				lv_subject_set_int(&subj_Second, st_DateTimeData.u1_Second);
			}
			if (st_DateTimeData.u1_Day != st_DateTimeData_Before.u1_Day) {
				lv_subject_set_int(&subj_day, st_DateTimeData.u1_Day);
			}
			if (st_DateTimeData.u1_Month != st_DateTimeData_Before.u1_Month) {
				lv_subject_set_int(&subj_month, st_DateTimeData.u1_Month);
			}
			if (st_DateTimeData.u1_DayOfWeek != st_DateTimeData_Before.u1_DayOfWeek) {
				lv_subject_set_int(&subj_weekday, st_DateTimeData.u1_DayOfWeek);
			}
			st_DateTimeData_Before = st_DateTimeData;

			char buffer[64];
			strftime(buffer, sizeof(buffer), "%H:%M:%S %d/%m/%Y", &timeinfo);
			ESP_LOGI("RTC", "Local Time: %s", buffer);
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
void Http_Task(void *pvParameter)
{
	http_init();
	while (1)
	{
		if (u1_WifiConnected_F == U1ON && u1_HttpRequest == U1ON)
		{
			u1_HttpRequest = U1OFF;
			http_rest_with_url();
		}
		vTaskDelay(5);
	}
}
void UI_task(void *pvParameters)
{
	UI_Init();		// Init UI
	while (1)
	{
		UI_Job();
		vTaskDelay(10);
	}
}
void Wifi_task(void *pvParameters)
{
	Init_WifiScan();      // Init WiFi scan
	while (1)
	{
		WifiScan_Job(); // Perform WiFi scan job
		vTaskDelay(10);
	}
}
void app_main(void)
{
	GC9A01_Init();    // Init LCD driver
	CS816D_Init();    // Init touch driver
	xTaskCreatePinnedToCore(Wifi_task, 	"Wifi_task", 	4096, NULL, 3, NULL, 0);
	xTaskCreatePinnedToCore(UI_task, 	"UI_task",		8192, NULL, 7, NULL, 0);
	xTaskCreatePinnedToCore(Http_Task, 	"Http_Task",	8192, NULL, 6, NULL, 0);
	xTaskCreatePinnedToCore(Rtc_Task, 	"Rtc_Task",		4096, NULL, 5, NULL, 0);

}