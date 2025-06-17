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
#include <stdio.h>
#include "HTTP_Client.h"
#include "esp_tls.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "Define.h"
#include "string.h"
#include "RamDef.h"
#include "cJSON.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>
//==================================================================================================
//	Local define
//==================================================================================================
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define OPEN_WEATHER_API_KEY	"b84c92063426d24ade911c7c7a771248"

//==================================================================================================
//	Local define I/O
//==================================================================================================

//==================================================================================================
//	Local Struct Template
//==================================================================================================

//==================================================================================================
//	Local RAM
//==================================================================================================
static esp_http_client_handle_t client;
static time_t now;
static struct tm timeinfo;

//==================================================================================================
//	Local ROM
//==================================================================================================
static const char *TAG = "HTTP_CLIENT";
//==================================================================================================
//	Local Function Prototype
//==================================================================================================
static esp_err_t _http_event_handler(esp_http_client_event_t *evt);
static void wait_for_time_sync();
static void ParseWeatherData(U1* apu1_WeatherData);
static void get_and_log_Time(void);
//==================================================================================================
//	Source Code
//==================================================================================================

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	http_init
//	Function:	Init HTTP client and SNTP time sync
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void http_init(void)
{
	u1_HttpRequest = U1ON;
	st_WeatherData.Lat = 0;
	st_WeatherData.Lon = 0;
	st_WeatherData.Temperature = 0;
	st_WeatherData.Humidity = 0;
	st_WeatherData.u1_GetWeatherDataSuccess_F = U1FALSE;

	st_DateTimeData.u1_Second = 0;
	st_DateTimeData.u1_Hour = 0;
	st_DateTimeData.u1_Minute = 0;
	st_DateTimeData.u1_DayOfWeek = 0;
	st_DateTimeData.u1_Day = 0;
	st_DateTimeData.u1_Month = 0;
	st_DateTimeData.u1_SynTimeDataSuccess_F = U1FALSE;

	memset(u1_BufferRespone, 0, sizeof(u1_BufferRespone));
	// Init http client and server end point to get weather data
	esp_http_client_config_t config = {
		.host = "api.openweathermap.org",
		.path = "/data/2.5/weather",
		.query = "lat=21.0285&lon=105.8542&appid=b84c92063426d24ade911c7c7a771248",
		.event_handler = _http_event_handler,
		.user_data = u1_BufferRespone,
		.disable_auto_redirect = true,
	};
	client = esp_http_client_init(&config);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	_http_event_handler
//	Function:	HTTP client event handler
//	
//	Argument:	esp_http_client_event_t *evt
//	Return	:	esp_err_t
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Handles HTTP client events such as connection, data reception, and errors.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	static char *output_buffer;  // Buffer to store response of http request from event handler
	static int output_len;       // Stores number of bytes read
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			// Clean the buffer in case of a new request
			if (output_len == 0 && evt->user_data) {
				// we are just starting to copy the output data into the use
				memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
			}
			/*
			 *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
			 *  However, event handler can also be used in case chunked encoding is used.
			 */
			if (!esp_http_client_is_chunked_response(evt->client)) {
				// If user_data buffer is configured, copy the response into the buffer
				int copy_len = 0;
				if (evt->user_data) {
					// The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
					copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
					if (copy_len) {
						memcpy(evt->user_data + output_len, evt->data, copy_len);
					}
				} else {
					int content_len = esp_http_client_get_content_length(evt->client);
					if (output_buffer == NULL) {
						// We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
						output_buffer = (char *) calloc(content_len + 1, sizeof(char));
						output_len = 0;
						if (output_buffer == NULL) {
							ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
							return ESP_FAIL;
						}
					}
					copy_len = MIN(evt->data_len, (content_len - output_len));
					if (copy_len) {
						memcpy(output_buffer + output_len, evt->data, copy_len);
					}
				}
				output_len += copy_len;
			}

			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			if (output_buffer != NULL) {
				// Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
				// ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
			int mbedtls_err = 0;
			esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
			if (err != 0) {
				ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
				ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
			}
			if (output_buffer != NULL) {
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_REDIRECT:
			ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
			esp_http_client_set_header(evt->client, "From", "user@example.com");
			esp_http_client_set_header(evt->client, "Accept", "text/html");
			esp_http_client_set_redirection(evt->client);
			break;
	}
	return ESP_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	Get_WeatherData
//	Function:	Trigger HTTP request to get weather data
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Sets the request flag to ON for weather data retrieval.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void Get_WeatherData(void)
{
	u1_HttpRequest = U1ON;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	ParseWeatherData
//	Function:	Parse JSON weather data and update st_WeatherData
//	
//	Argument:	U1* apu1_WeatherData - pointer to JSON data buffer
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Parses temperature and humidity from OpenWeatherMap JSON.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void ParseWeatherData(U1* apu1_WeatherData)
{
	cJSON *root = cJSON_Parse((char*)apu1_WeatherData);
	if (!root) {
		ESP_LOGE("WEATHER", "Failed to parse JSON");
		return;
	}

	cJSON *main = cJSON_GetObjectItem(root, "main");
	if (main) {
		st_WeatherData.Temperature = cJSON_GetObjectItem(main, "temp")->valuedouble;
		st_WeatherData.Humidity = cJSON_GetObjectItem(main, "humidity")->valuedouble;

		st_WeatherData.Temperature -= (double)273.15;
		ESP_LOGI("WEATHER", "Temp: %.2fÅãC, Humidity: %.2f%%", (double) st_WeatherData.Temperature, st_WeatherData.Humidity);
	}
	cJSON_Delete(root);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	http_rest_with_url
//	Function:	Perform HTTP GET request and process weather data
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Performs HTTP GET, parses weather data, logs result, and updates time.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void http_rest_with_url(void)
{
	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		st_WeatherData.u1_GetWeatherDataSuccess_F = U1TRUE;
		ParseWeatherData(u1_BufferRespone);
		ESP_LOGI(TAG, "Weather Data: %s", u1_BufferRespone);
		memset(u1_BufferRespone, 0, sizeof(u1_BufferRespone));
	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
	}
	// Init Simple network for get date time data
	get_and_log_Time();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	get_and_log_Time
//	Function:	Get current time, update system time data, and log it
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Gets and logs the current time, updates st_DateTimeData.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void get_and_log_Time(void)
{
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	esp_sntp_init();
	wait_for_time_sync();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	wait_for_time_sync
//	Function:	Waits for SNTP time synchronization to complete
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Blocks until time is synced or timeout occurs.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void wait_for_time_sync()
{
    time_t now;
    struct tm timeinfo;
    int retry = 0;
    const int retry_count = 10;

    time(&now);
    localtime_r(&now, &timeinfo);

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI("TIME", "Waiting for time sync... (%d)", retry);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2016 - 1900)) {
        char buffer[64];
        strftime(buffer, sizeof(buffer), "%c", &timeinfo);
		st_DateTimeData.u1_SynTimeDataSuccess_F = U1TRUE;
        ESP_LOGI("TIME", "Time synced: %s", buffer);
    } else {
		st_DateTimeData.u1_SynTimeDataSuccess_F = U1FALSE;
        ESP_LOGW("TIME", "Failed to sync time");
    }
}