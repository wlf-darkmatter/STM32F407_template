#ifndef __FUNCTION_WLF_H
#define __FUNCTION_WLF_H

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include <LED_STM32F407ZET6.h>
#include <Key_STM32F407ZET6.h>
#include <lcd.h>
#include <Beep.h>
#include "usmart.h"
#include <Tim.h>
#include <PWM.h>
#include <wlf_I2C.h>
#include <OLED.h>
#include <WLF_font.h>
#include <RTC.h>
#include <WIFI_ESP8266.h>
#include "SDIO_SDCard.h"
#include "malloc.h"
#include "includes.h"
#include "piclib.h"

#define USART1_BUSY 1//是否使能串口独占，0=不使用；1=使用
extern u8  USART1_Busy;					//如果为0，那就是被占用，除非被复位，否则不执行printf
/******************************* WiFi 部分 ********************************/
void WiFi_Debug(void);
void WiFi_Debug_task(void* pdata);






/*******************************图片部分********************************/
struct _app_LCD {
	u16 Picture_num;
};
extern struct _app_LCD App_LCD;
extern DIR PictureDir;
//获取图片数量
u8 PictureFile_Init(void);//记录所有图片信息
u16 pic_get_tnum(u8* path);


/*******************************OLED GUI部分********************************/
void OLED_GUIGRAM_Init(void);
void OLED_GUI_update(void* pdata);


/*******************************RTC**********************************/
extern RTC_TimeTypeDef RTC_TimeStruct;
extern RTC_DateTypeDef RTC_DateStruct;

#endif 
