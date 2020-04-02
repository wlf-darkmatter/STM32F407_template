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
extern u8  USART1_Busy;
/********************************  输入控制  ***********************************/
#define INPUT_TASK_PRIO				4
#define INPUT_STK_SIZE				32
extern OS_STK KEY_TASK_STK[INPUT_STK_SIZE];

void InputCommand_task(void* pdata);

/*--------------  KEY  -----------------*/
u8 Key_detect(void);

/*-------------  REMOTE  ---------------*/
u8 Remote_Scan(void);


/******************************* WiFi 部分 ********************************/
#define WIFI_DEBUG_TASK_PRIO				2
#define WIFI_DEBUG_STK_SIZE					128
#define WIFI_TASK_PRIO			8
#define WIFI_STK_SIZE			128

void WiFi_Debug(void);
void WiFi_Debug_task(void* pdata);

/*******************************       SD       ****************************************/
//如果SD卡错误，是否自动格式化，1=是，0=否
#define FORMAT_IF_ERROR 0
extern OS_EVENT* message_SD;			//SD卡读写邮箱事件块指针





/*******************************图片部分********************************/
struct _app_LCD {
	u16 Picture_num;
};
extern struct _app_LCD App_LCD;
extern DIR PictureDir;
//获取图片数量
u8 PictureFile_Init(void);//记录所有图片信息
u16 pic_get_tnum(u8* path);


/*******************************  OLED GUI部分  ********************************/
#define OLED_TASK_PRIO				6
#define OLED_STK_SIZE					128
extern OS_STK OLED_TASK_STK[OLED_STK_SIZE];
extern OS_EVENT* message_OLED;			//OLED卡读写邮箱事件块指针

void OLED_GUIGRAM_Init(void);
void OLED_GUI_update(void* pdata);


/*******************************RTC**********************************/
extern RTC_TimeTypeDef RTC_TimeStruct;
extern RTC_DateTypeDef RTC_DateStruct;
void Show_RTC(void);

/*******************************  USMART  **********************************/
#if USE_SMART_APP==1

#define USMART_APP_TASK_PRIO				1
#define USMART_APP_STK_SIZE					128
extern OS_STK USMART_APP_TASK_STK[USMART_APP_STK_SIZE];
void USMART_APP(void* pdata);

#endif
#endif 




/*****************************   REMOTE    *******************************/
#define RDATA PAin(8)	 //红外数据输入脚

//红外遥控识别码(ID),每款遥控器的该值基本都不一样,但也有一样的.
//我们选用的遥控器识别码为0
#define REMOTE_ID 0      		   

extern u8 RmtCnt;	//按键按下的次数
extern u32 RmtRec;
void Remote_Init(void);    //红外传感器接收头引脚初始化



