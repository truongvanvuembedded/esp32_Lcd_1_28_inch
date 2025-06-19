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
static U1 u1_TimerCnt;
//==================================================================================================
//	Local ROM
//==================================================================================================
//static const char *TAG = "main.c";
const char *month_names_short[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char *weekday_names_short[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
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
	st_DateTimeData.u1_Second = timeinfo.tm_sec;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			Rtc_Task
//	Name:			Rtc_Task
//	Function:		Update time, weather data, and location.
//					If any value changes compared to the previous one, it updates the UI subjects.
//
//	Argument:		- pvparameter: Not used (NULL)
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		Runs every 1 second, updates time, weather, and coordinates.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Rtc_Task(void* pvparameter)
{
	while (1)
	{
		// Weather update
		if (st_WeatherData.u1_GetWeatherDataSuccess_F == U1TRUE)
		{
			if (st_WeatherData.Temperature != st_WeatherData_Before.Temperature)
			{
				char buf[5];
				snprintf(buf, sizeof(buf), "%.1f", st_WeatherData.Temperature);
				lv_subject_copy_string(&subj_temperature, buf);
			}
			if (st_WeatherData.Humidity != st_WeatherData_Before.Humidity)
			{
				char buf[5];
				snprintf(buf, sizeof(buf), "%.1f", st_WeatherData.Humidity);
				lv_subject_copy_string(&subj_humidity, buf);
			}
			st_WeatherData_Before = st_WeatherData;
		}

		// Time update
		if (st_DateTimeData.u1_SynTimeDataSuccess_F == U1TRUE)
		{
			setenv("TZ", "ICT-7", 1);
			tzset();
			time(&now);
			localtime_r(&now, &timeinfo);
			update_TimeData(timeinfo);

			if (st_DateTimeData.u1_Hour != st_DateTimeData_Before.u1_Hour)
				lv_subject_set_int(&subj_hour, st_DateTimeData.u1_Hour);

			if (st_DateTimeData.u1_Minute != st_DateTimeData_Before.u1_Minute)
				lv_subject_set_int(&subj_minute, st_DateTimeData.u1_Minute);

			if (st_DateTimeData.u1_Second != st_DateTimeData_Before.u1_Second)
				lv_subject_set_int(&subj_Second, st_DateTimeData.u1_Second);

			if (st_DateTimeData.u1_Day != st_DateTimeData_Before.u1_Day)
				lv_subject_set_int(&subj_day, st_DateTimeData.u1_Day);

			if (st_DateTimeData.u1_Month != st_DateTimeData_Before.u1_Month)
				lv_subject_copy_string(&subj_month, month_names_short[st_DateTimeData.u1_Month - 1]);

			if (st_DateTimeData.u1_DayOfWeek != st_DateTimeData_Before.u1_DayOfWeek)
				lv_subject_copy_string(&subj_weekday, weekday_names_short[st_DateTimeData.u1_DayOfWeek]);

			st_DateTimeData_Before = st_DateTimeData;
		}

		// Location update
		if (st_LocationData.lat != st_LocationData_Before.lat)
		{
			char buf[8];
			snprintf(buf, sizeof(buf), "%.5f", st_LocationData.lat);
			lv_subject_copy_string(&subj_lat, buf);
		}
		if (st_LocationData.lon != st_LocationData_Before.lon)
		{
			char buf[8];
			snprintf(buf, sizeof(buf), "%.5f", st_LocationData.lon);
			lv_subject_copy_string(&subj_lon, buf);
		}
		st_LocationData_Before = st_LocationData;

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			Http_Task
//	Name:			Http_Task
//	Function:		Sends HTTP requests when WiFi is connected and request flag is set.
//
//	Argument:		- pvParameter: Not used (NULL)
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		Checks every 5ms for request trigger.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Http_Task(void *pvParameter)
{
	http_init();
	while (1)
	{
		// Get weather and time data if Wifi connected
		if (st_WifiStatus.u1_WifiConnected_F == U1ON)
		{
			if (u1_HttpRequest == U1ON)
			{
				u1_HttpRequest = U1OFF;
				http_rest_with_url();
			}
			if (u1_SynTimeRequest == U1ON)
			{
				u1_SynTimeRequest = U1OFF;
				sync_time_with_ntp();
			}
		}
		// Get period after 60 second
		if (u1_HttpRequest == U1OFF)
		{
			u1_TimerCnt++;
			if (u1_TimerCnt >= 60)
			{
				u1_TimerCnt = 0;
				u1_HttpRequest = U1ON;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			UI_task
//	Name:			UI_task
//	Function:		Initializes and runs the UI processing loop.
//
//	Argument:		- pvParameters: Not used (NULL)
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		Periodically calls UI_Job() every 10ms.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void UI_task(void *pvParameters)
{
	UI_Init();		// Init UI
	while (1)
	{
		UI_Job();
		vTaskDelay(pdMS_TO_TICKS(5));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			Wifi_task
//	Name:			Wifi_task
//	Function:		Initializes and continuously runs WiFi scan job.
//
//	Argument:		- pvParameters: Not used (NULL)
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		Runs WifiScan_Job every 10ms.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Wifi_task(void *pvParameters)
{
	Init_WifiScan();      // Init WiFi scan
	while (1)
	{
		WifiScan_Job(); // Perform WiFi scan job
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tag:			app_main
//	Name:			app_main
//	Function:		Main entry point of the application.
//					Initializes LCD, touch, and starts main FreeRTOS tasks.
//
//	Argument:		-
//	Return value:	-
//	Create:			2024.11.27 V.Vu	 New
//	Change:			-
//	Remarks:		Called once at system boot.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void app_main(void)
{
	GC9A01_Init();    // Init LCD driver
	CS816D_Init();    // Init touch driver
	xTaskCreatePinnedToCore(Wifi_task, 	"Wifi_task", 	4096, NULL, 3, NULL, 0);
	xTaskCreatePinnedToCore(UI_task, 	"UI_task",		8192, NULL, 7, NULL, 0);
	xTaskCreatePinnedToCore(Http_Task, 	"Http_Task",	8192, NULL, 6, NULL, 0);
	xTaskCreatePinnedToCore(Rtc_Task, 	"Rtc_Task",		4096, NULL, 5, NULL, 0);
}