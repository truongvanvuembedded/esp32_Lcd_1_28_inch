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
//==================================================================================================
//	RAM area (global variable definition/declaration)
//==================================================================================================

// main.c
EXTERN ST_TOUCH_DATA st_TouchData;
EXTERN U2 u2_WifiNum;	// Number of WiFi networks found
EXTERN ST_WIFI_INFO st_WifiInfo[SCAN_LIST_SIZE];	// Array to hold WiFi information

// Wifi_Scan.c
EXTERN U1 u1_WifiScanState;			// State of WiFi scan (off, on process, on complete)
EXTERN U1 u1_WifiScanState_Last;	// State of WiFi scan last(off, on process, on complete)
EXTERN U1 u1_WifiScanDone_F;		// Flag to indicate if WiFi scan is done
EXTERN U1 u1_WifiConnected_F;		// Flag to indicate if WiFi is connected
EXTERN U1 u1_WifiConnected_Fail_F;
// UI.c
EXTERN ST_WIFI_SELECTED st_WifiSelected;	// Struct to hold selected WiFi information
/* ************************************* End of File ******************************************** */
