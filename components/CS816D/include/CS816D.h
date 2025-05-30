#ifndef CS816D_H
#define CS816D_H
//==================================================================================================
//
//	File Name	:	CS816D.h
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
#include "Define.h"
//==================================================================================================
//	Local define
//==================================================================================================
#define U1_None			((U1)0x00)
#define U1_SlideDown	((U1)0x01)
#define U1_SlideUp		((U1)0x02)
#define U1_SlideLeft	((U1)0x03)
#define U1_SlideRight	((U1)0x04)
#define U1_SingleTap	((U1)0x05)
#define U1_DoubleTap	((U1)0x0B)
#define U1_LongPress	((U1)0x0C)
//==================================================================================================
//	Local define I/O
//==================================================================================================

//==================================================================================================
//	Local Struct Template
//==================================================================================================
// Touch data structure
typedef struct {
    U1 u1_Gesture;
    U1 u1_FingerNum;
    U2 u2_X;
    U2 u2_Y;
    U1 u1_Pressed;
} ST_TOUCH_DATA;
//==================================================================================================
//	Local RAM
//==================================================================================================

//==================================================================================================
//	Local ROM
//==================================================================================================

//==================================================================================================
//	Local Function Prototype
//==================================================================================================


//==================================================================================================
//	Source Code
//==================================================================================================

void CS816D_Init(void);
U1 u1_CS816D_ReadTouch(ST_TOUCH_DATA *apst_TouchData);
#endif