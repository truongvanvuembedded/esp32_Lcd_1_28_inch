//==================================================================================================
//
//	File Name	:	GC9A01.c
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
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"

#include "sdkconfig.h"
#include "GC9A01.h"
#include "Define.h"
#include "RamDef.h"
//==================================================================================================
//	Local define
//==================================================================================================
#if (CONFIG_GC9A01_RESET_USED)
#define RESET_HIGH()		gpio_set_level(GC9A01_PIN_NUM_RST,1)
#define RESET_LOW()			gpio_set_level(GC9A01_PIN_NUM_RST,0)
#endif

#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_USED)
#define BLK_HIGH()			gpio_set_level(GC9A01_PIN_NUM_SCK,1)
#define BLK_LOW()			gpio_set_level(GC9A01_PIN_NUM_SCK,0)
#endif
				
// Sleep mode commands
#define U1_CMD_SLPIN				((U1)0x10)   // Enter sleep mode
#define U1_CMD_SLPOUT				((U1)0x11)   // Exit sleep mode

// Display inversion control
#define U1_CMD_INVOFF				((U1)0x20)   // Display inversion OFF
#define U1_CMD_INVON				((U1)0x21)   // Display inversion ON

// Display ON/OFF control
#define U1_CMD_DISPOFF				((U1)0x28)   // Display OFF
#define U1_CMD_DISPON				((U1)0x29)   // Display ON

// Column and row address set
#define U1_CMD_CASET				((U1)0x2A)   // Column address set
#define U1_CMD_RASET				((U1)0x2B)   // Row address set
#define U1_CMD_RAMWR				((U1)0x2C)   // Write to RAM

// Tearing effect and memory access control
#define U1_CMD_TEON				 ((U1)0x35)   // Tearing effect line ON
#define U1_CMD_MADCTL				((U1)0x36)   // Memory data access control

// Pixel format control
#define U1_CMD_COLMOD				((U1)0x3A)   // Pixel format set

// Display function control
#define U1_CMD_DISPLAY_FUNTION_CTRL		((U1)0xB6)   // Display function control

// Power control commands
#define U1_CMD_PWCTR1				((U1)0xC1)   // Power control 1
#define U1_CMD_PWCTR2				((U1)0xC3)   // Power control 2
#define U1_CMD_PWCTR3				((U1)0xC4)   // Power control 3
#define U1_CMD_PWCTR4				((U1)0xC9)   // Power control 4
#define U1_CMD_PWCTR7				((U1)0xA7)   // Power control 7

// Frame rate setting
#define U1_CMD_FRAMERATE			((U1)0xE8)   // Frame rate control

// Internal register access
#define U1_CMD_INNER_REG1_EN		((U1)0xFE)   // Enable inner register 1
#define U1_CMD_INNER_REG2_EN		((U1)0xEF)   // Enable inner register 2

// Gamma control
#define U1_CMD_GAMMA1				((U1)0xF0)   // Set gamma curve 1
#define U1_CMD_GAMMA2				((U1)0xF1)   // Set gamma curve 2
#define U1_CMD_GAMMA3				((U1)0xF2)   // Set gamma curve 3
#define U1_CMD_GAMMA4				((U1)0xF3)   // Set gamma curve 4

// Color mode options
#define U1_CMD_COLOR_MODE_RGB_16		((U1)0x50)   // RGB interface, 16-bit
#define U1_CMD_COLOR_MODE_RGB_18		((U1)0x60)   // RGB interface, 18-bit
#define U1_CMD_COLOR_MODE_MCU_12		((U1)0x03)   // MCU interface, 12-bit
#define U1_CMD_COLOR_MODE_MCU_16		((U1)0x05)   // MCU interface, 16-bit
#define U1_CMD_COLOR_MODE_MCU_18		((U1)0x06)   // MCU interface, 18-bit

// Memory Access Control flags
#define U1_CMD_MADCTL_MY				((U1)0x80)   // Row Address Order
#define U1_CMD_MADCTL_MX				((U1)0x40)   // Column Address Order
#define U1_CMD_MADCTL_MV				((U1)0x20)   // Row/Column Exchange
#define U1_CMD_MADCTL_ML				((U1)0x10)   // Vertical Refresh Order
#define U1_CMD_MADCTL_BGR				((U1)0x08)   // RGB-BGR Order
#define U1_CMD_MADCTL_MH				((U1)0x04)   // Horizontal Refresh Order
//==================================================================================================
//	Local define I/O
//==================================================================================================

//==================================================================================================
//	Local Struct Template
//==================================================================================================
/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
	U1 cmd;
	U1 data[16];
	U1 databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;
//==================================================================================================
//	Local RAM
//==================================================================================================
U1 u1_GC9A01_X_Start = 0, u1_GC9A01_Y_Start = 0;

#if (CONFIG_GC9A01_BUFFER_MODE)
#if (CONFIG_GC9A01_BUFFER_MODE_PSRAM)
U2 *u2_ScreenBuff = NULL;
#else
DMA_ATTR U2 u2_ScreenBuff[GC9A01_HEIGHT * GC9A01_WIDTH];
#endif
#endif

//SPI Config
spi_device_handle_t spi;
spi_host_device_t LCD_HOST=SPI2_HOST;

//LEDC Config
#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_USED)
#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_MODE)
ledc_channel_config_t  ledc_cConfig;
ledc_timer_config_t	ledc_tConfig;
static void LEDC_PWM_Duty_Set(U1 DutyP);
static void LEDC_Channel_Config(void);
static void LEDC_Timer_Config(void);
#endif
#endif
//==================================================================================================
//	Local ROM
//==================================================================================================
//static const char *TAG = "GC9A01";
// Command sequence to init LCD
static const lcd_init_cmd_t lcd_init_cmds[]={
	{0xef,{0},0},
	{0xeb,{0x14},1},
	{0xfe,{0},0},
	{0xef,{0},0},
	{0xeb,{0x14},1},
	{0x84,{0x40},1},
	{0x85,{0xff},1},
	{0x86,{0xff},1},
	{0x87,{0xff},1},
	{0x88,{0x0a},1},
	{0x89,{0x21},1},
	{0x8a,{0x00},1},
	{0x8b,{0x80},1},
	{0x8c,{0x01},1},
	{0x8d,{0x01},1},
	{0x8e,{0xff},1},
	{0x8f,{0xff},1},
	{U1_CMD_DISPLAY_FUNTION_CTRL,{0x00,0x20},2},// Scan direction S360 -> S1
	//{U1_CMD_MADCTL,{0x08},1},//MemAccessModeSet(0, 0, 0, 1);
	//{U1_CMD_COLMOD,{U1_CMD_COLOR_MODE_MCU_16&0x77},1},
	{0x90,{0x08,0x08,0x08,0x08},4},
	{0xbd,{0x06},1},
	{0xbc,{0x00},1},
	{0xff,{0x60,0x01,0x04},3},
	{U1_CMD_PWCTR2,{0x13},1},
	{U1_CMD_PWCTR3,{0x13},1},
	{U1_CMD_PWCTR4,{0x22},1},
	{0xbe,{0x11},1},
	{0xe1,{0x10,0x0e},2},
	{0xdf,{0x21,0x0c,0x02},3},
	{U1_CMD_GAMMA1,{0x45,0x09,0x08,0x08,0x26,0x2a},6},
	{U1_CMD_GAMMA2,{0x43,0x70,0x72,0x36,0x37,0x6f},6},
	{U1_CMD_GAMMA3,{0x45,0x09,0x08,0x08,0x26,0x2a},6},
	{U1_CMD_GAMMA4,{0x43,0x70,0x72,0x36,0x37,0x6f},6},
	{0xed,{0x1b,0x0b},2},
	{0xae,{0x77},1},
	{0xcd,{0x63},1},
	{0x70,{0x07,0x07,0x04,0x0e,0x0f,0x09,0x07,0x08,0x03},9},
	{U1_CMD_FRAMERATE,{0x34},1},// 4 dot inversion
	{0x62,{0x18,0x0D,0x71,0xED,0x70,0x70,0x18,0x0F,0x71,0xEF,0x70,0x70},12},
	{0x63,{0x18,0x11,0x71,0xF1,0x70,0x70,0x18,0x13,0x71,0xF3,0x70,0x70},12},
	{0x64,{0x28,0x29,0xF1,0x01,0xF1,0x00,0x07},7},
	{0x66,{0x3C,0x00,0xCD,0x67,0x45,0x45,0x10,0x00,0x00,0x00},10},
	{0x67,{0x00,0x3C,0x00,0x00,0x00,0x01,0x54,0x10,0x32,0x98},10},
	{0x74,{0x10,0x85,0x80,0x00,0x00,0x4E,0x00},7},
	{0x98,{0x3e,0x07},2},
	{U1_CMD_TEON,{0},0},// Tearing effect line on
	{0, {0}, 0xff},//END
};
//==================================================================================================
//	Local Function Prototype
//==================================================================================================
static void gc9a01_GPIO_init(void);
static void gc9a01_SPI_init(void);
static void lcd_spi_pre_transfer_callback(spi_transaction_t *t);
static void GC9A01_SleepMode(U1 Mode);
static void GC9A01_InversionMode(U1 Mode) ;
static void GC9A01_DisplayPower(U1 On);
static void ColumnSet(U2 ColumnStart, U2 ColumnEnd);
static void RowSet(U2 RowStart, U2 RowEnd) ;
static void ColorModeSet(U1 ColorMode) ;
static void MemAccessModeSet(U1 Rotation, U1 VertMirror,
		U1 HorizMirror, U1 IsBGR);
static U2 GC9A01_GetPixel(int16_t x, int16_t y);
static void SwapBytes(U2 *color);
static void GC9A01_HardReset(void);
static void delay_ms (uint32_t Delay_ms);
static void lcd_cmd(U1 cmd);
//==================================================================================================
//	Source Code
//==================================================================================================
static U1 u1_cst816d_i2c_read_continous(U1 u1_RegAdd, U1 *apu1_Data, U2 au2_Len);
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_Init
//	Function:	Init I2C for CS816D
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GC9A01_Init()
{
	int cmd=0;
	u1_BrightNessValue = 100;
	u1_GC9A01_X_Start = 0;
	u1_GC9A01_Y_Start = 0;

	#if (CONFIG_GC9A01_BUFFER_MODE_PSRAM)
	if (u2_ScreenBuff == NULL) {
		// ScreenBuffer has not yet been allocated
		u2_ScreenBuff = heap_caps_malloc((GC9A01_HEIGHT * GC9A01_WIDTH) * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT );
	}
	#endif

	gc9a01_GPIO_init();
	gc9a01_SPI_init();

	#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_USED)
	#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_MODE)
	LEDC_Timer_Config();
	LEDC_Channel_Config();
	#endif
	#endif

	#if(CONFIG_GC9A01_RESET_USED)
	GC9A01_HardReset();
	#endif

	//Send all the commands
	while (lcd_init_cmds[cmd].databytes!=0xff)
	{
		lcd_cmd(lcd_init_cmds[cmd].cmd);
		lcd_data(lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes&0x1F);
		if (lcd_init_cmds[cmd].databytes&0x80)
		{
			delay_ms(100);
		}
		cmd++;
	}

	MemAccessModeSet(0,0,0,1);
	ColorModeSet(U1_CMD_COLOR_MODE_MCU_16);

	GC9A01_InversionMode(1);
	GC9A01_SleepMode(0);

	delay_ms(120);
	GC9A01_DisplayPower(1);
	delay_ms(20);

	#if(CONFIG_GC9A01_BUFFER_MODE)
	GC9A01_Clear();
	GC9A01_Update();
	delay_ms(30);
	#endif

	#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_USED)
	GC9A01_SetBL(u1_BrightNessValue);
	#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_Init
//	Function:	Init GPIO for gc9a01
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void gc9a01_GPIO_init(void)
{
	gpio_config_t gpiocfg={
		.pin_bit_mask= ((uint64_t)1UL<<GC9A01_PIN_NUM_DC),
		.mode=GPIO_MODE_OUTPUT,
		.pull_up_en=GPIO_PULLUP_DISABLE,
		.pull_down_en=GPIO_PULLDOWN_DISABLE,
		.intr_type=GPIO_INTR_DISABLE,
	};

	gpio_config(&gpiocfg);
	gpio_set_level(GC9A01_PIN_NUM_DC,0);

	#if(CONFIG_GC9A01_RESET_USED)
	gpiocfg.pin_bit_mask|=((uint64_t)1UL<<GC9A01_PIN_NUM_RST);
	gpio_config(&gpiocfg);
	gpio_set_level(GC9A01_PIN_NUM_RST,1);
	#endif
	//gpiocfg.pin_bit_mask|=((uint64_t)1UL<<GC9A01_PIN_NUM_LED_CTRL);
	//gpio_config(&gpiocfg);
	//gpio_set_level(3,1);



	#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_USED)
	#if(!CONFIG_GC9A01_CONTROL_BACK_LIGHT_MODE)
	gpiocfg.pin_bit_mask|=((uint64_t)1UL<<GC9A01_PIN_NUM_SCK);
	gpio_config(&gpiocfg);
	gpio_set_level(GC9A01_PIN_NUM_SCK,0);
	#endif
	#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	gc9a01_SPI_init
//	Function:	Init SPI for gc9a01
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void gc9a01_SPI_init(void)
{
	// Config SPI bus
	esp_err_t ret;
	spi_bus_config_t buscfg={
		.mosi_io_num=GC9A01_PIN_NUM_MOSI,
		.miso_io_num=GPIO_NUM_NC,
		.sclk_io_num=GC9A01_PIN_NUM_SCK,
		.quadwp_io_num=-1,
		.quadhd_io_num=-1,
		.max_transfer_sz=250*250*2,
	};
	ret=spi_bus_initialize(LCD_HOST,&buscfg,SPI_DMA_CH_AUTO);
	ESP_ERROR_CHECK(ret);
	// Add device to bus
	spi_device_interface_config_t devcfg={
		.clock_speed_hz=GC9A01_SPI_SCK_FREQ_M,
		.mode=0,
		.spics_io_num=GC9A01_PIN_NUM_CS,
		.queue_size=7,
		.pre_cb=lcd_spi_pre_transfer_callback,
	};
	ret=spi_bus_add_device(LCD_HOST,&devcfg,&spi);
	ESP_ERROR_CHECK(ret);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	lcd_spi_pre_transfer_callback
//	Function:	This function is called (in irq context!) just before a transmission starts. It will
//				set the D/C line to the value indicated in the user field.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static IRAM_ATTR void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
	int dc=(int)t->user;
	gpio_set_level(GC9A01_PIN_NUM_DC, dc);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	lcd_cmd
//	Function:	Send a command to the LCD. Uses spi_device_polling_transmit, which waits
//				until the transfer is complete.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Since command transactions are usually small, they are handled in polling
// 				mode for higher speed. The overhead of interrupt transactions is more than
// 				just waiting for the transaction to complete.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void lcd_cmd(U1 cmd)
{
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));	   //Zero out the transaction
	t.length=8;					 //Command is 8 bits
	t.tx_buffer=&cmd;			   //The data is the cmd itself
	t.user=(void*)0;				//D/C needs to be set to 0
	ret=spi_device_polling_transmit(spi, &t);  //Transmit!
	assert(ret==ESP_OK);			//Should have had no issues.
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	lcd_data
//	Function:	Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
//				transfer is complete.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	09/05/2025
//	Change	:	-
//	Remarks	:	Since data transactions are usually small, they are handled in polling
// 				mode for higher speed. The overhead of interrupt transactions is more than
// 				just waiting for the transaction to complete.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void lcd_data(const U1 *data, int len)
{
	esp_err_t ret;
	if (len==0) return;			 //no need to send anything

	/*
	On certain MC's the max SPI DMA transfer length might be smaller than the buffer. We then have to split the transmissions.
	*/
	int offset = 0;
	do {
		spi_transaction_t t;
		memset(&t, 0, sizeof(t));	   //Zero out the transaction

		int tx_len = ((len - offset) < SPI_MAX_DMA_LEN) ? (len - offset) : SPI_MAX_DMA_LEN;
		t.length=tx_len * 8;					   //Len is in bytes, transaction length is in bits.
		t.tx_buffer= data + offset;				//Data
		t.user=(void*)1;						   //D/C needs to be set to 1
		ret=spi_device_polling_transmit(spi, &t);  //Transmit!
		assert(ret==ESP_OK);					   //Should have had no issues.
		offset += tx_len;						  // Add the transmission length to the offset
	}
	while (offset < len);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	lcd_send_byte
//	Function:	Send a single byte to the display via lcd_data function.
//	
//	Argument:	Data ? the 8-bit value to send to the display
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void lcd_send_byte(U1 Data)
{
	lcd_data(&Data, 1);					// Send one byte to the LCD using lcd_data()
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	delay_ms
//	Function:	Delay execution for the given number of milliseconds.
//	
//	Argument:	Delay_ms ? number of milliseconds to delay
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Wrapper over FreeRTOS vTaskDelay
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void delay_ms(uint32_t Delay_ms)
{
	vTaskDelay(Delay_ms / portTICK_PERIOD_MS);	// Convert ms to OS ticks and delay
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_GetWidth
//	Function:	Return the width of the display in pixels.
//	
//	Argument:	-
//	Return	:	Display width (U2)
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
U2 GC9A01_GetWidth() {
	return GC9A01_WIDTH;				// Return constant width defined elsewhere
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_GetHeight
//	Function:	Return the height of the display in pixels.
//	
//	Argument:	-
//	Return	:	Display height (U2)
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
U2 GC9A01_GetHeight() {
	return GC9A01_HEIGHT;				// Return constant height defined elsewhere
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_HardReset
//	Function:	Perform a hard reset on the GC9A01 display.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Only executed if reset pin is enabled via config.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void GC9A01_HardReset(void) {
	#if (CONFIG_GC9A01_RESET_USED)
	RESET_LOW();						// Drive reset pin low
	delay_ms(10);						// Wait 10ms
	RESET_HIGH();						// Drive reset pin high
	delay_ms(150);						// Wait 150ms for display to initialize
	#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_SleepMode
//	Function:	Enter or exit sleep mode.
//	
//	Argument:	Mode ? 1 to enter sleep mode, 0 to wake
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Requires delay after mode change.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void GC9A01_SleepMode(U1 Mode) 
{
	if (Mode)
		lcd_cmd(U1_CMD_SLPIN);			// Send sleep-in command
	else
		lcd_cmd(U1_CMD_SLPOUT);			// Send sleep-out command

	delay_ms(500);						// Wait for sleep mode transition
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_InversionMode
//	Function:	Enable or disable display color inversion.
//	
//	Argument:	Mode ? 1 to enable inversion, 0 to disable
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Used for visual effect
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void GC9A01_InversionMode(U1 Mode) 
{
	if (Mode)
		lcd_cmd(U1_CMD_INVON);			// Enable inversion
	else
		lcd_cmd(U1_CMD_INVOFF);			// Disable inversion
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_DisplayPower
//	Function:	Turn the display on or off.
//	
//	Argument:	On ? 1 to turn on, 0 to turn off
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void GC9A01_DisplayPower(U1 On) {
	if (On)
		lcd_cmd(U1_CMD_DISPON);			// Turn display on
	else
		lcd_cmd(U1_CMD_DISPOFF);		// Turn display off
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	ColumnSet
//	Function:	Set the column address window for subsequent pixel data.
//	
//	Argument:	ColumnStart ? start column
//				ColumnEnd ? end column
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void ColumnSet(U2 ColumnStart, U2 ColumnEnd) 
{
	if (ColumnStart > ColumnEnd)
		return;							// Invalid range
	if (ColumnEnd > GC9A01_WIDTH)
		return;							// Out of bounds

	ColumnStart += u1_GC9A01_X_Start;	// Apply offset
	ColumnEnd += u1_GC9A01_X_Start;

	lcd_cmd(U1_CMD_CASET);				// Set column address command
	lcd_send_byte(ColumnStart >> 8);	// High byte of start
	lcd_send_byte(ColumnStart & 0xFF);	// Low byte of start
	lcd_send_byte(ColumnEnd >> 8);		// High byte of end
	lcd_send_byte(ColumnEnd & 0xFF);	// Low byte of end
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	RowSet
//	Function:	Set the row address window for subsequent pixel data.
//	
//	Argument:	RowStart ? start row
//				RowEnd ? end row
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	-
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void RowSet(U2 RowStart, U2 RowEnd) 
{
	if (RowStart > RowEnd)
		return;							// Invalid range
	if (RowEnd > GC9A01_HEIGHT)
		return;							// Out of bounds

	RowStart += u1_GC9A01_Y_Start;		// Apply offset
	RowEnd += u1_GC9A01_Y_Start;

	lcd_cmd(U1_CMD_RASET);				// Set row address command
	lcd_send_byte(RowStart >> 8);		// High byte of start
	lcd_send_byte(RowStart & 0xFF);		// Low byte of start
	lcd_send_byte(RowEnd >> 8);			// High byte of end
	lcd_send_byte(RowEnd & 0xFF);		// Low byte of end
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_SetWindow
//	Function:	Define a rectangular area of the display RAM for pixel updates.
//	
//	Argument:	x0, y0 ? top-left corner
//				x1, y1 ? bottom-right corner
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	After calling, you can write pixel data into this window
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GC9A01_SetWindow(U1 x0, U1 y0, U1 x1, U1 y1) 
{
	ColumnSet(x0, x1);					// Set column range
	RowSet(y0, y1);						// Set row range

	lcd_cmd(U1_CMD_RAMWR);				// Prepare to write to RAM
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	ColorModeSet
//	Function:	Set color format (e.g., 16-bit, 18-bit).
//	
//	Argument:	ColorMode ? bitmask of color mode settings
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Only certain values are valid (e.g., 0x55 for RGB565)
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void ColorModeSet(U1 ColorMode) 
{
	lcd_cmd(U1_CMD_COLMOD);				// Color mode command
	lcd_send_byte(ColorMode & 0x77);	// Mask unsupported bits
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	MemAccessModeSet
//	Function:	Set display orientation, mirror and BGR modes.
//	
//	Argument:	Rotation ? display rotation (0-7)
//				VertMirror ? vertical mirror enable
//				HorizMirror ? horizontal mirror enable
//				IsBGR ? use BGR instead of RGB
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Useful for adjusting display direction
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void MemAccessModeSet(U1 Rotation, U1 VertMirror, U1 HorizMirror, U1 IsBGR) {
	U1 Ret = 0;
	Rotation &= 7;						// Limit rotation to 3 bits

	lcd_cmd(U1_CMD_MADCTL);				// Memory access control command

	switch (Rotation) {
	case 0: Ret = 0; break;
	case 1: Ret = U1_CMD_MADCTL_MX; break;
	case 2: Ret = U1_CMD_MADCTL_MY; break;
	case 3: Ret = U1_CMD_MADCTL_MX | U1_CMD_MADCTL_MY; break;
	case 4: Ret = U1_CMD_MADCTL_MV; break;
	case 5: Ret = U1_CMD_MADCTL_MV | U1_CMD_MADCTL_MX; break;
	case 6: Ret = U1_CMD_MADCTL_MV | U1_CMD_MADCTL_MY; break;
	case 7: Ret = U1_CMD_MADCTL_MV | U1_CMD_MADCTL_MX | U1_CMD_MADCTL_MY; break;
	}

	if (VertMirror)
		Ret = U1_CMD_MADCTL_ML;			// Enable vertical mirror
	if (HorizMirror)
		Ret = U1_CMD_MADCTL_MH;			// Enable horizontal mirror
	if (IsBGR)
		Ret |= U1_CMD_MADCTL_BGR;		// Enable BGR color order

	lcd_send_byte(Ret);					// Send configuration
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_SetBL
//	Function:	Set the backlight brightness or state.
//	
//	Argument:	Value ? 0 to 100 (%)
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Config-dependent implementation
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GC9A01_SetBL(U1 Value)
{
	if (Value > 100) Value = 100;		// Cap to 100%

	#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_USED)
		#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_MODE)
		LEDC_PWM_Duty_Set(Value);		// Set PWM duty for backlight
		#else
		if (Value) BLK_HIGH();			// Turn backlight on
		else BLK_LOW();					// Turn backlight off
		#endif
	#endif
}

//Direct Mode

#if (!CONFIG_GC9A01_BUFFER_MODE)
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_RamWrite
//	Function:	Write a sequence of pixel data directly to the display RAM.
//	
//	Argument:	pBuff ? pointer to the buffer containing pixel data
//				Len   ? number of pixels to write
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Used in direct mode (no buffer)
//
////////////////////////////////////////////////////////////////////////////////////////////////////
	void GC9A01_RamWrite(U2 *pBuff, U2 Len)
	{
		while (Len--)
		{
			lcd_send_byte(*pBuff >> 8);
			lcd_send_byte(*pBuff & 0xFF);
		}
	}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_DrawPixel
//	Function:	Draw a single pixel at specified coordinates.
//	
//	Argument:	x, y   ? coordinates of the pixel
//				color ? pixel color (U2)
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Direct mode version
//
////////////////////////////////////////////////////////////////////////////////////////////////////
	void GC9A01_DrawPixel(int16_t x, int16_t y, U2 color)
	{
		if ((x < 0) ||(x >= GC9A01_WIDTH) || (y < 0) || (y >= GC9A01_HEIGHT))
			return;

		GC9A01_SetWindow(x, y, x, y);
		GC9A01_RamWrite(&color, 1);
	}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_FillRect
//	Function:	Fill a rectangle area with a specific color.
//	
//	Argument:	x, y     ? top-left corner coordinates
//				w, h     ? width and height of the rectangle
//				color    ? fill color (U2)
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Direct mode version
//
////////////////////////////////////////////////////////////////////////////////////////////////////
	void GC9A01_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, U2 color)
	{
		if ((x >= GC9A01_WIDTH) || (y >= GC9A01_HEIGHT))
			return;

		if ((x + w) > GC9A01_WIDTH)
			w = GC9A01_WIDTH - x;

		if ((y + h) > GC9A01_HEIGHT)
			h = GC9A01_HEIGHT - y;

		GC9A01_SetWindow(x, y, x + w - 1, y + h - 1);

		for (uint32_t i = 0; i < (h * w); i++)
			GC9A01_RamWrite(&color, 1);
	}

//Buffer mode
#else
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	SwapBytes
//	Function:	Swap the byte order of a 16-bit color value.
//	
//	Argument:	color ? pointer to the color value
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Used for endian correction
//
////////////////////////////////////////////////////////////////////////////////////////////////////
	static void SwapBytes(U2 *color) {
		U1 temp = *color >> 8;
		*color = (*color << 8) | temp;
	}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_DrawPixel
//	Function:	Draw a pixel to the screen buffer.
//	
//	Argument:	x, y   ? coordinates of the pixel
//				color ? pixel color (U2)
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Buffer mode version
//
////////////////////////////////////////////////////////////////////////////////////////////////////
	void GC9A01_DrawPixel(int16_t x, int16_t y, U2 color) {
		if ((x < 0) || (x >= GC9A01_WIDTH) || (y < 0) || (y >= GC9A01_HEIGHT))
			return;

		SwapBytes(&color);

		u2_ScreenBuff[y * GC9A01_WIDTH + x] = color;
	}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_GetPixel
//	Function:	Get the color of a pixel from the screen buffer.
//	
//	Argument:	x, y ? coordinates of the pixel
//	Return	:	Pixel color (U2)
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Buffer mode version
//
////////////////////////////////////////////////////////////////////////////////////////////////////
	static U2 GC9A01_GetPixel(int16_t x, int16_t y) {
		if ((x < 0) || (x >= GC9A01_WIDTH) || (y < 0) || (y >= GC9A01_HEIGHT))
			return 0;

		U2 color = u2_ScreenBuff[y * GC9A01_WIDTH + x];
		SwapBytes(&color);
		return color;
	}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_FillRect
//	Function:	Fill a rectangular area in the buffer with a specific color.
//	
//	Argument:	x, y     ? top-left corner coordinates
//				w, h     ? width and height
//				color    ? fill color (U2)
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Buffer mode version
//
////////////////////////////////////////////////////////////////////////////////////////////////////
	void GC9A01_FillRect(int16_t x, int16_t y, int16_t w, int16_t h,
			U2 color) {
		if ((w <= 0) || (h <= 0) || (x >= GC9A01_WIDTH) || (y >= GC9A01_HEIGHT))
			return;

		if (x < 0) {
			w += x;
			x = 0;
		}
		if (y < 0) {
			h += y;
			y = 0;
		}

		if ((w <= 0) || (h <= 0))
			return;

		if ((x + w) > GC9A01_WIDTH)
			w = GC9A01_WIDTH - x;
		if ((y + h) > GC9A01_HEIGHT)
			h = GC9A01_HEIGHT - y;

		SwapBytes(&color);

		for (U2 row = 0; row < h; row++) {
			for (U2 col = 0; col < w; col++)
				//GC9A01_DrawPixel(col, row, color);
				u2_ScreenBuff[(y + row) * GC9A01_WIDTH + x + col] = color;
		}
	}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_Update
//	Function:	Send the entire buffer content to the display.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Buffer mode only
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GC9A01_Update()
{
	int len = GC9A01_WIDTH * GC9A01_HEIGHT;
	GC9A01_SetWindow(0, 0, GC9A01_WIDTH - 1, GC9A01_HEIGHT - 1);
	lcd_data((U1*) &u2_ScreenBuff[0], len*2);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_Clear
//	Function:	Clear the screen by filling it with black color.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Buffer mode only
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GC9A01_Clear(void)
{
	GC9A01_FillRect(0, 0, GC9A01_WIDTH, GC9A01_HEIGHT, 0x0000);
}

#endif

#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_USED)
#if(CONFIG_GC9A01_CONTROL_BACK_LIGHT_MODE)
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	LEDC_PWM_Duty_Set
//	Function:	Set PWM duty cycle for backlight control.
//	
//	Argument:	DutyP ? duty percentage (0 to 100)
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Config-dependent
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void LEDC_PWM_Duty_Set(U1 DutyP)
{
	U2 Duty,MaxD;

	MaxD=(1<<(int)ledc_tConfig.duty_resolution)-1;

	if(DutyP>=100)Duty=MaxD;
	else
	{
		Duty=DutyP*(MaxD/(float)100);
	}
	ledc_cConfig.duty=Duty;
	ledc_channel_config(&ledc_cConfig);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	LEDC_Channel_Config
//	Function:	Configure LEDC channel for PWM output.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Backlight control
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void LEDC_Channel_Config(void)
{
	ledc_cConfig.gpio_num=GC9A01_PIN_NUM_LED_CTRL;
	ledc_cConfig.speed_mode=LEDC_LOW_SPEED_MODE;
	ledc_cConfig.channel=LEDC_CHANNEL_0;
	ledc_cConfig.intr_type=LEDC_INTR_DISABLE;
	ledc_cConfig.timer_sel=LEDC_TIMER_0;
	ledc_cConfig.duty=0;
	ledc_cConfig.hpoint=0;
	ledc_channel_config(&ledc_cConfig);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	LEDC_Timer_Config
//	Function:	Configure timer settings for LEDC PWM.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Backlight control
//
////////////////////////////////////////////////////////////////////////////////////////////////////
static void LEDC_Timer_Config(void)
{
	ledc_tConfig.speed_mode=LEDC_LOW_SPEED_MODE ;
	ledc_tConfig.duty_resolution=LEDC_TIMER_8_BIT;
	//ledc_tConfig.bit_num=LEDC_TIMER_8_BIT;
	ledc_tConfig.timer_num=LEDC_TIMER_0;
	ledc_tConfig.freq_hz=1000;
	ledc_tConfig.clk_cfg=LEDC_AUTO_CLK;
	ledc_timer_config(&ledc_tConfig);
}
#endif
#endif



#if(CONFIG_GC9A01_BUFFER_MODE)
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_Free
//	Function:	Free the PSRAM buffer used for the screen.
//	
//	Argument:	-
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Only used in PSRAM mode
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#if (CONFIG_GC9A01_BUFFER_MODE_PSRAM)
void GC9A01_Free(void) {
	if (u2_ScreenBuff != NULL) {
		free(u2_ScreenBuff);
	}
}
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_Screen_Shot
//	Function:	Capture a portion of the screen into a buffer.
//	
//	Argument:	x, y       ? top-left position to capture
//				width, height ? size of the region
//				Buffer    ? output buffer
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Buffer mode only
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GC9A01_Screen_Shot(U2 x,U2 y,U2 width ,U2 height,U2 * Buffer)
{
	U2 i,j;
	for (i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			#if(!CONFIG_GC9A01_BUFFER_SCREEN_FAST_MODE)
			Buffer[i*width+j]=GC9A01_GetPixel(x+j,y+i);
			#else
			Buffer[i*width+j]=u2_ScreenBuff[((y+i) * GC9A01_WIDTH )+ (x+j)];
			#endif
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Name	:	GC9A01_Screen_Load
//	Function:	Load pixel data from a buffer into the screen.
//	
//	Argument:	x, y       ? top-left position to draw
//				width, height ? size of the data
//				Buffer    ? input buffer
//	Return	:	-
//	Create	:	20/05/2025
//	Change	:	-
//	Remarks	:	Buffer mode only
//
////////////////////////////////////////////////////////////////////////////////////////////////////
void GC9A01_Screen_Load(U2 x,U2 y,U2 width ,U2 height,U2 * Buffer)
{
	U2 i,j;
	for (i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			#if(!CONFIG_GC9A01_BUFFER_SCREEN_FAST_MODE)
			GC9A01_DrawPixel(x+j,y+i,Buffer[i*width+j]);
			#else
			u2_ScreenBuff[((y+i) * GC9A01_WIDTH )+ (x+j)] = Buffer[i*width+j];
			#endif
		}
	}
}

#endif
