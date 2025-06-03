//==================================================================================================
//
// 	File Name		DataDef.h
//
//	CPU Type		ESP32-S3
//	Builder
//
//	Coding
//
//	Outline
//
//
//	History			Ver.0.01	2025.02.05 V.Vu	New
//
//==================================================================================================
//==================================================================================================
//	Local Compile Option
//==================================================================================================
#ifndef DATA_DEF_H
#define DATA_DEF_H

#include "Define.h"
//==================================================================================================
//	Declare constants that are commonly used in all files
//==================================================================================================
// Size for the WiFi scan list
#define SCAN_LIST_SIZE 10
// Wifi scan state
#define WIFI_SCAN_OFF			((U1)0)
#define WIFI_SCAN_ON			((U1)1)
//==================================================================================================
//	Declare a structure template that is commonly used for all files
//==================================================================================================
// Struct for Wifi information
typedef struct 
{
	U1 u1_ssid[32];		// SSID of the WiFi network
	U1 u1_bssid[6];		// BSSID of the WiFi network (MAC address)
	U1 u1_channel;		// Channel of the WiFi network
	U1 u1_rssi;			// Signal strength of the WiFi network
	U1 u1_authmode;		// Authentication mode of the WiFi network
} ST_WIFI_INFO;

typedef struct
{
	U1 u1_ssid[32];		// Pointer to the SSID of the selected WiFi network
	U1 u1_password[64];	// Pointer to the WiFi password
	U1 u1_WifiPasswordValid_F;	// Flag to indicate if the WiFi password is valid
} ST_WIFI_SELECTED;

#endif	// DATA_DEF_H
/* ************************************* End of File ******************************************** */
