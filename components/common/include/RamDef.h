//==================================================================================================
//
// 	File Name		RamDef.h
//
//	CPU Type		ESP32-S3
//	Builder
//
//	Coding
//
//	Outline
//
//
//	History			Ver.0.01	// 2025.02.05 V.Vu	New
//
//==================================================================================================
//==================================================================================================
//	Local Compile Option
//==================================================================================================
#ifdef REAL_RAM
#define EXTERN
#else
#define EXTERN		extern
#endif

#include "Define.h"
#include "CS816D.h"
#include "DataDef.h"
#include "lvgl.h"
//==================================================================================================
//	RAM area (global variable definition/declaration)
//==================================================================================================

// main.c
EXTERN ST_TOUCH_DATA st_TouchData;
EXTERN U2 u2_WifiNum;	// Number of WiFi networks found
EXTERN ST_WIFI_INFO st_WifiInfo[SCAN_LIST_SIZE];	// Array to hold WiFi information

// Wifi_Scan.c
EXTERN ST_WIFI_STATUS st_WifiStatus;
// UI.c
EXTERN ST_WIFI_SELECTED st_WifiSelected;	// Struct to hold selected WiFi information
EXTERN lv_subject_t subj_day;
EXTERN lv_subject_t subj_month;
EXTERN lv_subject_t subj_weekday;
EXTERN lv_subject_t subj_hour;
EXTERN lv_subject_t subj_minute;
EXTERN lv_subject_t subj_Second;
EXTERN lv_subject_t subj_lat;
EXTERN lv_subject_t subj_lon;
EXTERN lv_subject_t subj_temperature;
EXTERN lv_subject_t subj_humidity;
// HTTP_Client.c
EXTERN U1 u1_BufferRespone[MAX_HTTP_OUTPUT_BUFFER + 1];
EXTERN U1 u1_HttpRequest;
EXTERN U1 u1_SynTimeRequest;

EXTERN ST_WEATHER_DATA st_WeatherData;
EXTERN ST_WEATHER_DATA st_WeatherData_Before;
EXTERN ST_DATETIME_DATA st_DateTimeData;
EXTERN ST_DATETIME_DATA st_DateTimeData_Before;
EXTERN ST_LOCATION_DATA st_LocationData;
EXTERN ST_LOCATION_DATA st_LocationData_Before;
// GC9A01.c
EXTERN U1 u1_BrightNessValue;

/* ************************************* End of File ******************************************** */
