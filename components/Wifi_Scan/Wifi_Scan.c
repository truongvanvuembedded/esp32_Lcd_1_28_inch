//==================================================================================================
//
//	File Name	:	Wifi_Scan.c
//	CPU Type	:	ESP32
//	IDE			:	ESP-IDF V5.3.1
//	Customer	
//	Version		:	Ver.0.01
//	Coding		:	v.Vu
//	History		:	09/05/2025
//	Outline		:
//
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
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "regex.h"
#include "Wifi_Scan.h"
#include "Define.h"
#include "RamDef.h"
//==================================================================================================
//	Local define
//==================================================================================================
#define U1_ESP_MAXIMUM_RETRY_CONNECT ((U1)3)	// Maximum number of connection retries
//==================================================================================================
//	Local define I/O
//==================================================================================================

//==================================================================================================
//	Local Struct Template
//==================================================================================================

//==================================================================================================
//	Local RAM
//==================================================================================================
static U1 u1_ConnectRetry_num;
//==================================================================================================
//	Local ROM
//==================================================================================================
static const char *TAG = "Wifi_Scan";
//==================================================================================================
//	Local Function Prototype
//==================================================================================================

//==================================================================================================
//	Source Code
//==================================================================================================
static void Wifi_Scan_Start(void);
static void Wifi_Scan_Stop(void);
static void Wifi_Connect(void);
static void Wifi_DisConnect(void);
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	Init_WifiScan
//	Function:	Init Wifi scan functionality
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Init_WifiScan(void)
{
	// Initialize WiFi num
	u2_WifiNum = U2MIN;
	u1_ConnectRetry_num = U1MIN;
	u1_WifiConnected_F = U1FALSE; // Initialize WiFi connected flag to FALSE
	u1_WifiConnected_Fail_F = U1FALSE;
	// Initialize WiFi scan state
	u1_WifiScanState = WIFI_SCAN_OFF;			// Initialize WiFi scan state to OFF
	u1_WifiScanState_Last = WIFI_SCAN_OFF;	// Initialize last WiFi scan state to OFF
	u1_WifiScanDone_F = U1FALSE;
	
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	// Initialize WiFi
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	WifiScan_Job
//	Function:	Perform Wifi scan job follow Switch state
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void WifiScan_Job(void)
{
	// Check if WiFi scan state has changed
	if(u1_WifiScanState != u1_WifiScanState_Last) 
	{
		if(u1_WifiScanState == WIFI_SCAN_ON) 
		{
			// Start WiFi scan
			Wifi_Scan_Start();
		} 
		else if(u1_WifiScanState == WIFI_SCAN_OFF) 
		{
			// Stop WiFi scan
			Wifi_Scan_Stop();
		}
		u1_WifiScanState_Last = u1_WifiScanState; // Update last WiFi scan state
	}
	Wifi_Connect(); // Wait Uset typing WiFi password and connect to the selected WiFi network
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	Wifi_Scan_Start
//	Function:	Start Wifi scan functionality
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void Wifi_Scan_Start(void)
{
	wifi_ap_record_t ap_info[SCAN_LIST_SIZE];
	uint16_t ap_count = 0;
	memset(ap_info, 0, sizeof(ap_info));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	esp_wifi_scan_start(NULL, true);

	u2_WifiNum = SCAN_LIST_SIZE;
	//ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", SCAN_LIST_SIZE);
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&u2_WifiNum, ap_info));
	//ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, u2_WifiNum);
	for (U1 au1_ForC = 0; au1_ForC < u2_WifiNum; au1_ForC++) {
		memcpy(st_WifiInfo[au1_ForC].u1_ssid, ap_info[au1_ForC].ssid, sizeof(ap_info[au1_ForC].ssid));
		st_WifiInfo[au1_ForC].u1_rssi = ap_info[au1_ForC].rssi;
		memcpy(st_WifiInfo[au1_ForC].u1_bssid, ap_info[au1_ForC].bssid, sizeof(ap_info[au1_ForC].bssid));
		st_WifiInfo[au1_ForC].u1_channel = ap_info[au1_ForC].primary;
		st_WifiInfo[au1_ForC].u1_authmode = ap_info[au1_ForC].authmode;
	}
	u1_WifiScanDone_F = U1TRUE; // Set WiFi scan done flag
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	Wifi_Scan_Stop
//	Function:	Stop Wifi scan functionality
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void Wifi_Scan_Stop(void)
{
	u1_WifiScanDone_F = U1FALSE; // Reset WiFi scan done flag
	u2_WifiNum = U1MIN; // Reset WiFi number
	memset(st_WifiInfo, 0, sizeof(st_WifiInfo));
	u1_WifiConnected_F = U1FALSE; // Reset WiFi connected flag
	ESP_ERROR_CHECK(esp_wifi_stop());
	ESP_LOGI(TAG, "WiFi scan stopped and cleaned up.");
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	Wifi_Connect
//	Function:	Connect to the selected WiFi network
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void Wifi_Connect(void)
{
	if (st_WifiSelected.u1_WifiPasswordValid_F == U1TRUE)
	{
		if (u1_WifiConnected_F == U1TRUE)
		{
			return;
		}
		wifi_config_t wifi_config = {0};

		strcpy((char *)wifi_config.sta.ssid, (const char *)st_WifiSelected.u1_ssid);
		strcpy((char *)wifi_config.sta.password, (const char *)st_WifiSelected.u1_password);
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
		esp_wifi_connect();

		st_WifiSelected.u1_WifiPasswordValid_F = U1FALSE; // Reset lại flag
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	event_handler
//	Function:	Event handler for WiFi and IP events
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void event_handler(void* arg, esp_event_base_t event_base,
								int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		// esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		u1_WifiConnected_F = U1FALSE; // Reset WiFi connected flag
		u1_WifiConnected_Fail_F = U1FALSE;
		if (u1_ConnectRetry_num < U1_ESP_MAXIMUM_RETRY_CONNECT) {
			esp_wifi_connect();
			u1_ConnectRetry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		}
		else {
			ESP_LOGI(TAG, "connect to the AP fail");
			u1_WifiConnected_Fail_F = U1TRUE;
			u1_ConnectRetry_num = 0; // Reset retry count
		}
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		u1_WifiConnected_Fail_F = U1FALSE;
		u1_WifiConnected_F = U1TRUE; // Set WiFi connected flag
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		u1_ConnectRetry_num = 0;
	}
}